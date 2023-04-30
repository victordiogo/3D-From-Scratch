#ifndef _3D_FROM_SCRATCH_RENDERER_HPP
#define _3D_FROM_SCRATCH_RENDERER_HPP

#include "model.hpp"
#include "timer.hpp"

#include <SFML/Graphics.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <utility>
#include <vector>

inline auto is_vertice_inside_frustum(const glm::vec4& v) -> bool
{
  // after w division, to be inside the frustum, each normalized coordinate c in (x, y, z) should be ruled for: -1 <= c <= 1
  return -v.w <= v.x && v.x <= v.w
         && -v.w <= v.y && v.y <= v.w
         && -v.w <= v.z && v.z <= v.w;
}

inline auto is_vertice_outside_frustum(const glm::vec4& v) -> bool
{
  return !is_vertice_inside_frustum(v);
}

// accepts vertices in clip space
inline auto is_triangle_outside_frustum(const glm::vec4& vert_a, const glm::vec4& vert_b, const glm::vec4& vert_c) -> bool
{
  return is_vertice_outside_frustum(vert_a)
         && is_vertice_outside_frustum(vert_b)
         && is_vertice_outside_frustum(vert_c);
}

inline auto is_triangle_inside_frustum(const glm::vec4& vert_a, const glm::vec4& vert_b, const glm::vec4& vert_c) -> bool
{
  return is_vertice_inside_frustum(vert_a)
         && is_vertice_inside_frustum(vert_b)
         && is_vertice_inside_frustum(vert_c);
}

enum FrustumPlane {
  back,
  front,
  left,
  bottom,
  right,
  top,
  plane_count
};

inline auto dot_product(const glm::vec4& v, FrustumPlane fplane) -> float
{
  switch (fplane) {
    case back:
      return v.z + v.w;
    case front:
      return -v.z + v.w;
    case left:
      return v.x + v.w;
    case right:
      return -v.x + v.w;
    case bottom:
      return v.y + v.w;
    case top:
      return -v.y + v.w;
    default:
      assert(false && "Invalid frustum plane");
      return 1.0F;
  }
}

// accepts vertices in clip space
inline auto clip_triangle(const glm::vec4& vert_a, const glm::vec4& vert_b, const glm::vec4& vert_c) -> std::vector<glm::vec4>
{
  auto input = std::vector<glm::vec4>{ vert_a, vert_b, vert_c };
  auto output = std::vector<glm::vec4>{};
  output.reserve(5);
  for (auto fplane = 0; fplane < plane_count; ++fplane) {
    auto in_size = input.size();
    auto last_index = in_size - 1;
    auto last_d = dot_product(input[last_index], static_cast<FrustumPlane>(fplane));
    for (auto index = 0UZ; index < in_size; ++index) {
      auto d = dot_product(input[index], static_cast<FrustumPlane>(fplane));
      if (last_d >= 0.0f) {
        output.push_back(input[last_index]);
        if (d < 0.0f) {
          auto t = last_d / (last_d - d);
          assert(t >= 0.0F && t <= 1.0F);
          output.push_back(glm::lerp(input[last_index], input[index], t));
        }
      }
      else if (d >= 0.0f) {
        auto t = last_d / (last_d - d);
        assert(t >= 0.0F && t <= 1.0F);
        output.push_back(glm::lerp(input[last_index], input[index], t));
      }
      last_index = index;
      last_d = d;
    }
    std::swap(input, output);
    output.clear();
  }
  assert(input.size() >= 3);
  return input;
}

class Renderer final {
public:
  friend bool update_window(sf::RenderWindow& window, const Renderer& renderer);

  Renderer(std::size_t render_width, std::size_t render_height)
    : m_render_width{ render_width },
      m_render_height{ render_height },
      m_colors(render_width * render_height)
  {
  }

  void render(const Model& model)
  {
    static auto s_timer = Timer{};
    auto model_matrix = glm::rotate(glm::mat4{ 1.0F }, static_cast<float>(s_timer.elapsed()) / 5000.0F, glm::vec3{ 0.0F, 1.0F, 0.0F });
    // model_matrix = glm::translate(model_matrix, { -3, 0, 0 });
    auto view_matrix = glm::lookAt(glm::vec3{ 0.0F, 1.5F, 5.0F }, glm::vec3{ 0.0F, 1.5F, 0.0 }, glm::vec3{ 0.0F, -1.0F, 0.0F });
    auto projection_matrix = glm::perspective(glm::radians(60.0F), static_cast<float>(m_render_width) / static_cast<float>(m_render_height), 0.1F, 100.0F);
    auto transform_matrix = projection_matrix * view_matrix * model_matrix;
    for (const auto& mesh : model.meshes) {
      for (const auto& face : mesh.faces) {
        auto vert_a = transform_matrix * glm::vec4{ face[0].position, 1.0F };
        auto vert_b = transform_matrix * glm::vec4{ face[1].position, 1.0F };
        auto vert_c = transform_matrix * glm::vec4{ face[2].position, 1.0F };
        if (is_triangle_outside_frustum(vert_a, vert_b, vert_c)) continue;
        if (is_triangle_inside_frustum(vert_a, vert_b, vert_c)) {
          render_triangle(vert_a, vert_b, vert_c);
        }
        else {
          // interpolate texture coordinates
          auto clipped_verts = clip_triangle(vert_a, vert_b, vert_c);
          for (auto index = 2UZ; index < clipped_verts.size(); ++index) {
            render_triangle(clipped_verts[0], clipped_verts[index - 1], clipped_verts[index]);
          }
        }
      }
    }
  }

  void render_triangle(glm::vec4 vert_a, glm::vec4 vert_b, glm::vec4 vert_c)
  {
    assert(vert_a.w != 0.0F && vert_b.w != 0.0F && vert_c.w != 0.0F);
    vert_a /= vert_a.w;
    vert_a.x += 1.0F;
    vert_a.x *= static_cast<float>(m_render_width) * 0.5F;
    vert_a.y += 1.0F;
    vert_a.y *= static_cast<float>(m_render_height) * 0.5F;
    vert_b /= vert_b.w;
    vert_b.x += 1.0F;
    vert_b.x *= static_cast<float>(m_render_width) * 0.5F;
    vert_b.y += 1.0F;
    vert_b.y *= static_cast<float>(m_render_height) * 0.5F;
    vert_c /= vert_c.w;
    vert_c.x += 1.0F;
    vert_c.x *= static_cast<float>(m_render_width) * 0.5F;
    vert_c.y += 1.0F;
    vert_c.y *= static_cast<float>(m_render_height) * 0.5F;

    auto min_bounds = glm::ivec2{ 0 };
    auto max_bounds = glm::ivec2{ m_render_width - 1, m_render_height - 1 };
    auto scr_a = glm::clamp(glm::ivec2{ vert_a.x, vert_a.y }, min_bounds, max_bounds);
    auto scr_b = glm::clamp(glm::ivec2{ vert_b.x, vert_b.y }, min_bounds, max_bounds);
    auto scr_c = glm::clamp(glm::ivec2{ vert_c.x, vert_c.y }, min_bounds, max_bounds);

    plot_line(scr_a, scr_b);
    plot_line(scr_b, scr_c);
    plot_line(scr_c, scr_a);
  }

  void clear()
  {
    static auto s_background = std::vector<std::uint32_t>(m_render_width * m_render_height, 0xFF000000);
    m_colors = s_background;
  }

  void plot(std::size_t x, std::size_t y, std::uint32_t color = 0xFFBFBFBF)
  {
    assert(x < m_render_width && y < m_render_height);
    m_colors[y * m_render_width + x] = color;
  }

  void plot_vertical_line(glm::ivec2 begin, const glm::ivec2& end)
  {
    auto increment{ begin.y > end.y ? -1 : 1 };
    while (true) {
      plot(
        static_cast<std::size_t>(begin.x),
        static_cast<std::size_t>(begin.y));
      if (begin.y == end.y) break;
      begin.y += increment;
    }
  }

  void plot_horizontal_line(glm::ivec2 begin, const glm::ivec2& end)
  {
    auto increment{ begin.x > end.x ? -1 : 1 };
    while (true) {
      plot(
        static_cast<std::size_t>(begin.x),
        static_cast<std::size_t>(begin.y));
      if (begin.x == end.x) break;
      begin.x += increment;
    }
  }

  void plot_line(glm::ivec2 begin, const glm::ivec2& end)
  {
    if (end.x == begin.x) plot_vertical_line(begin, end);
    if (end.y == begin.y) plot_horizontal_line(begin, end);
    auto distance_x{ std::abs(end.x - begin.x) };
    auto distance_y{ -std::abs(end.y - begin.y) };
    auto increment_x{ begin.x < end.x ? 1 : -1 };
    auto increment_y{ begin.y < end.y ? 1 : -1 };
    auto error{ distance_x + distance_y };
    // auto t{ 0.0 }; // interpolation parameter
    // auto t_increment{ 1.0 / std::sqrt(distance_x * distance_x + distance_y * distance_y) };
    while (true) {
      plot(
        static_cast<std::size_t>(begin.x),
        static_cast<std::size_t>(begin.y));
      if (begin.x == end.x && begin.y == end.y) break;
      auto error2{ 2 * error };
      if (error2 >= distance_y) {
        if (begin.x == end.x) break;
        error += distance_y;
        begin.x += increment_x;
      }
      if (error2 <= distance_x) {
        if (begin.y == end.y) break;
        error += distance_x;
        begin.y += increment_y;
      }
      // t += t_increment;
    }
  }

  std::size_t render_width() const { return m_render_width; }
  std::size_t render_height() const { return m_render_height; }

private:
  std::size_t m_render_width{};
  std::size_t m_render_height{};
  std::vector<std::uint32_t> m_colors{};
};

#endif