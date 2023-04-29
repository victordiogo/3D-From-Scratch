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

inline auto is_vertice_outside_frustum(const glm::vec4& v) -> bool
{
  // after w division, to be inside the frustum, each coordinate c (x, y, z) should be ruled for: -1 <= c <= 1
  // in clip space: -w <= c <= w
  return (-v.w > v.x || v.x > v.w)
         && (-v.w > v.y || v.y > v.w)
         && (-v.w > v.z || v.z > v.w);
}

// accepts vertices in clip space
inline auto is_triangle_outside_frustum(const glm::vec4& vert_a, const glm::vec4& vert_b, const glm::vec4& vert_c) -> bool
{
  return is_vertice_outside_frustum(vert_a)
         && is_vertice_outside_frustum(vert_b)
         && is_vertice_outside_frustum(vert_c);
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
inline auto clip_triangle(std::list<glm::vec4>&& input) -> std::list<glm::vec4>
{
  std::list<glm::vec4> output{};
  for (auto fplane = 0; fplane < FrustumPlane::plane_count; ++fplane) {
    for (auto it = input.begin(); it != input.end(); ++it) {
      auto d0 = dot_product(*it, static_cast<FrustumPlane>(fplane));
      auto next = std::next(it);
      if (next == input.end()) {
        next = input.begin();
      }
      auto d1 = dot_product(*next, static_cast<FrustumPlane>(fplane));
      if (d0 >= 0.0f) {
        output.push_back(std::move(*it));
        if (d1 < 0.0f) {
          auto t = d0 / (d0 - d1);
          assert(t >= 0.0F && t <= 1.0F);
          output.push_back(glm::lerp(output.back(), *next, t));
        }
      }
      else if (d1 >= 0.0f) {
        auto t = d0 / (d0 - d1);
        assert(t >= 0.0F && t <= 1.0F);
        output.push_back(glm::lerp(*it, *next, t));
      }
    }
    // Output is never empty. This case is handled before this function is called
    // if (output.empty()) {
    //   return {};
    // }
    input.clear();
    std::swap(output, input);
  }
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

  void render(const Model& model [[maybe_unused]])
  {
    static auto s_timer = Timer{};
    // model -> world -> camera -> projection -> screen
    auto model_matrix = glm::rotate(glm::mat4{ 1.0F }, static_cast<float>(s_timer.elapsed()) / 5000.0F, glm::vec3{ 0.0F, 1.0F, 0.0F });
    auto view_matrix = glm::lookAt(glm::vec3{ 0.0F, 1.5F, 5.0F }, glm::vec3{ 0.0F, 1.5F, 0.0 }, glm::vec3{ 0.0F, -1.0F, 0.0F });
    auto projection_matrix = glm::perspective(glm::radians(60.0F), static_cast<float>(m_render_width) / static_cast<float>(m_render_height), 0.1F, 100.0F);
    auto transform_matrix = projection_matrix * model_matrix * view_matrix * model_matrix;
    for (const auto& mesh : model.meshes) {
      for (const auto& face : mesh.faces) {
        auto vert_a = transform_matrix * glm::vec4{ face[0].position, 1.0F };
        auto vert_b = transform_matrix * glm::vec4{ face[1].position, 1.0F };
        auto vert_c = transform_matrix * glm::vec4{ face[2].position, 1.0F };
        if (is_triangle_outside_frustum(vert_a, vert_b, vert_c)) continue;
        auto clipped_verts = clip_triangle(std::list<glm::vec4>{ std::move(vert_a), std::move(vert_b), std::move(vert_c) });
        for (auto it = std::next(clipped_verts.begin()); it != std::prev(clipped_verts.end()); ++it) {
          auto vert_a = *clipped_verts.begin();
          auto vert_b = *it;
          auto vert_c = *std::next(it);
          render_triangle(vert_a, vert_b, vert_c);
        }
        // pos_a /= pos_a.w;
        // pos_a.x += 1.0F;
        // pos_a.x *= static_cast<float>(m_render_width) * 0.5F;
        // pos_a.y += 1.0F;
        // pos_a.y *= static_cast<float>(m_render_height) * 0.5F;
        // pos_b /= pos_b.w;
        // pos_b.x += 1.0F;
        // pos_b.x *= static_cast<float>(m_render_width) * 0.5F;
        // pos_b.y += 1.0F;
        // pos_b.y *= static_cast<float>(m_render_height) * 0.5F;
        // pos_c /= pos_c.w;
        // pos_c.x += 1.0F;
        // pos_c.x *= static_cast<float>(m_render_width) * 0.5F;
        // pos_c.y += 1.0F;
        // pos_c.y *= static_cast<float>(m_render_height) * 0.5F;

        // [[maybe_unused]] auto scr_a = glm::ivec2{ pos_a.x, pos_a.y };
        // [[maybe_unused]] auto scr_b = glm::ivec2{ pos_b.x, pos_b.y };
        // [[maybe_unused]] auto scr_c = glm::ivec2{ pos_c.x, pos_c.y };

        // plot_line(scr_a, scr_b);
        // plot_line(scr_b, scr_c);
        // plot_line(scr_c, scr_a);
      }
    }
  }

  void render_triangle(glm::vec4& vert_a, glm::vec4& vert_b, glm::vec4& vert_c)
  {
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

    auto scr_a = glm::clamp(glm::ivec2{ vert_a.x, vert_a.y }, glm::ivec2{ 0 }, glm::ivec2{ m_render_width - 1, m_render_height - 1 });
    auto scr_b = glm::clamp(glm::ivec2{ vert_b.x, vert_b.y }, glm::ivec2{ 0 }, glm::ivec2{ m_render_width - 1, m_render_height - 1 });
    auto scr_c = glm::clamp(glm::ivec2{ vert_c.x, vert_c.y }, glm::ivec2{ 0 }, glm::ivec2{ m_render_width - 1, m_render_height - 1 });

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
    if (x >= m_render_width || y >= m_render_height) {
      std::cerr << "x: " << x << " y: " << y << '\n';
    }
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