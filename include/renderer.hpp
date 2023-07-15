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
#include <stdlib.h>
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
      assert(false && "Invalid enum frustum plane");
      return 1.0F;
  }
}

struct ClipVertex {
  glm::vec4 position{};
  glm::vec2 texture_coord{};
};

struct ScreenVertex {
  glm::vec2 position{};
  float inv_z{};
  glm::vec2 texture_coord{};
};

inline void order_by_ascending_y(ScreenVertex& vert_a, ScreenVertex& vert_b, ScreenVertex& vert_c)
{
  if (vert_b.position.y < vert_a.position.y) {
    std::swap(vert_a, vert_b);
  }
  if (vert_c.position.y < vert_b.position.y) {
    std::swap(vert_b, vert_c);
  }
  if (vert_b.position.y < vert_a.position.y) {
    std::swap(vert_a, vert_b);
  }
}

inline auto to_screen_vertex(const ClipVertex& v, int width, int height) -> ScreenVertex
{
  assert(v.position.w != 0.0F);
  auto norm_x = v.position.x / v.position.w;
  auto norm_y = v.position.y / v.position.w;
  auto inv_z = v.position.w / v.position.z;
  auto max_width = static_cast<float>(width - 1);
  auto max_height = static_cast<float>(height - 1);
  return {
    glm::vec2{
      glm::clamp(
        std::floor((norm_x * 0.5F + 0.5F) * static_cast<float>(max_width)), 0.0F, max_width),
      glm::clamp(
        std::floor((norm_y * 0.5F + 0.5F) * static_cast<float>(max_height)), 0.0F, max_height) },
    inv_z,
    v.texture_coord * inv_z,
  };
}

// accepts vertices in clip space
inline auto clip_triangle(const ClipVertex& vert_a, const ClipVertex& vert_b, const ClipVertex& vert_c) -> std::vector<ClipVertex>
{
  auto input = std::vector<ClipVertex>{ vert_a, vert_b, vert_c };
  auto output = std::vector<ClipVertex>{};
  output.reserve(5);
  for (auto fplane = 0; fplane < plane_count; ++fplane) {
    auto in_size = input.size();
    auto last_index = in_size - 1;
    auto last_d = dot_product(input[last_index].position, static_cast<FrustumPlane>(fplane));
    for (auto index = 0UZ; index < in_size; ++index) {
      auto d = dot_product(input[index].position, static_cast<FrustumPlane>(fplane));
      if (last_d >= 0.0f) {
        output.push_back(input[last_index]);
        if (d < 0.0f) {
          auto t = last_d / (last_d - d);
          assert(t >= 0.0F && t <= 1.0F);
          output.push_back(ClipVertex{
            glm::lerp(input[last_index].position, input[index].position, t),
            glm::lerp(input[last_index].texture_coord, input[index].texture_coord, t) });
        }
      }
      else if (d >= 0.0f) {
        auto t = last_d / (last_d - d);
        assert(t >= 0.0F && t <= 1.0F);
        output.push_back(ClipVertex{
          glm::lerp(input[last_index].position, input[index].position, t),
          glm::lerp(input[last_index].texture_coord, input[index].texture_coord, t) });
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
      m_colors(render_width * render_height),
      m_depth(render_width * render_height)
  {
  }

  void render(const Model& model)
  {
    static auto s_timer = Timer{};
    // model_matrix = glm::translate(model_matrix, { 6 * glm::sin(s_timer.elapsed() / 2000.0), 0, 0 });
    auto model_matrix = glm::translate(glm::mat4{ 1.0F }, { 0, 0, -10 });
    model_matrix = glm::rotate(model_matrix, static_cast<float>(s_timer.elapsed()) * 0.0001F, glm::vec3{ 0.0F, 1.0F, 0.0F });
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
          render_triangle(
            ClipVertex{ vert_a, face[0].texture_coord },
            ClipVertex{ vert_b, face[1].texture_coord },
            ClipVertex{ vert_c, face[2].texture_coord },
            mesh.texture);
        }
        else {
          auto clipped_verts = clip_triangle(
            ClipVertex{ vert_a, face[0].texture_coord },
            ClipVertex{ vert_b, face[1].texture_coord },
            ClipVertex{ vert_c, face[2].texture_coord });
          for (auto index = 2UZ; index < clipped_verts.size(); ++index) {
            render_triangle(clipped_verts[0], clipped_verts[index - 1], clipped_verts[index], mesh.texture);
          }
        }
      }
    }
  }

  void render_triangle(const ClipVertex& clip_a, const ClipVertex& clip_b, const ClipVertex& clip_c, const Texture& texture)
  {
    auto width = static_cast<int>(m_render_width);
    auto height = static_cast<int>(m_render_height);
    auto a = to_screen_vertex(clip_a, width, height); // used to interpolate between a and c
    auto b = to_screen_vertex(clip_b, width, height);
    auto c = to_screen_vertex(clip_c, width, height);
    order_by_ascending_y(a, b, c);

    auto compare = [](auto a, auto b, auto tolerance) {
      return std::abs(a - b) < tolerance;
    };

    if (compare(a.position.y, c.position.y, 1.F)) {
      plot_horizontal_line(a, b, texture);
      plot_horizontal_line(a, c, texture);
      plot_horizontal_line(b, c, texture);
      return;
    }
    // need to handle the cases when a.y == b.y and when b.y == c.y
    // does the case b.y == c.y needs to be handled?
    auto ac_dy = c.position.y - a.position.y;
    auto ab_dy = b.position.y - a.position.y;
    auto bc_dy = c.position.y - b.position.y;

    auto inc_ac_x = (c.position.x - a.position.x) / ac_dy;
    auto inc_ac_inv_z = (c.inv_z - a.inv_z) / ac_dy;
    auto inc_ac_tex_coord = (c.texture_coord - a.texture_coord) / ac_dy;

    ScreenVertex var; // used to interpolate between a and b, then b to c
    float inc_var_x;
    float inc_var_inv_z;
    glm::vec2 inc_var_tex_coord;

    if (compare(a.position.y, b.position.y, 1.F)) {
      var = b;
      inc_var_x = (c.position.x - var.position.x) / bc_dy;
      inc_var_inv_z = (c.inv_z - var.inv_z) / bc_dy;
      inc_var_tex_coord = (c.texture_coord - var.texture_coord) / bc_dy;
    }
    else {
      var = a;
      inc_var_x = (b.position.x - a.position.x) / ab_dy;
      inc_var_inv_z = (b.inv_z - a.inv_z) / ab_dy;
      inc_var_tex_coord = (b.texture_coord - a.texture_coord) / ab_dy;
    }

    for (; static_cast<int>(a.position.y) <= static_cast<int>(c.position.y); ++a.position.y) {
      plot_horizontal_line(a, var, texture);
      if (static_cast<int>(a.position.y) == static_cast<int>(b.position.y)) {
        if (compare(b.position.y, c.position.y, 1.F)) {
          break;
        }
        inc_var_x = (c.position.x - var.position.x) / bc_dy;
        inc_var_inv_z = (c.inv_z - var.inv_z) / bc_dy;
        inc_var_tex_coord = (c.texture_coord - var.texture_coord) / bc_dy;
      }
      a.position.x += inc_ac_x;
      a.inv_z += inc_ac_inv_z;
      a.texture_coord += inc_ac_tex_coord;
      var.position.x += inc_var_x;
      var.inv_z += inc_var_inv_z;
      var.texture_coord += inc_var_tex_coord;
    }
  }

  void clear()
  {
    static auto s_background = std::vector<std::uint32_t>(m_render_width * m_render_height, 0xFF000000);
    static auto s_depth = std::vector<float>(m_render_width * m_render_height, 1000.F);
    m_colors = s_background;
    m_depth = s_depth;
  }

  void plot(std::size_t x, std::size_t y, std::uint32_t color = 0xFFDFDFDF)
  {
    assert(x < m_render_width && y < m_render_height);
    m_colors[y * m_render_width + x] = color;
  }

  void plot_horizontal_line(ScreenVertex beg, const ScreenVertex& end, const Texture& texture)
  {
    auto increment = beg.position.x > end.position.x ? -1.F : 1.F;
    auto dx = end.position.x - beg.position.x;
    auto inc_inv_z = (end.inv_z - beg.inv_z) / dx;
    auto inc_tex_coord = (end.texture_coord - beg.texture_coord) / dx;
    while (true) {
      auto z = 1 / beg.inv_z;
      auto tcoord = z * beg.texture_coord;
      if (z < m_depth[static_cast<unsigned>(beg.position.x) + m_render_width * static_cast<unsigned>(beg.position.y)]) {
        m_depth[static_cast<unsigned>(beg.position.x) + m_render_width * static_cast<unsigned>(beg.position.y)] = z;
        plot(
          static_cast<unsigned>(beg.position.x),
          static_cast<unsigned>(beg.position.y),
          texture[static_cast<std::size_t>(tcoord.x), static_cast<std::size_t>(tcoord.y)]);
      }
      if (static_cast<int>(beg.position.x) == static_cast<int>(end.position.x)) break;
      beg.position.x += increment;
      beg.inv_z += inc_inv_z;
      beg.texture_coord += inc_tex_coord;
    }
  }

  void plot_line(const glm::ivec2& begin, const glm::ivec2& end)
  {
    auto dx = end.x - begin.x;
    auto dy = end.y - begin.y;
    auto steps = std::max(abs(dx), abs(dy));
    auto fsteps = static_cast<float>(steps);
    auto x_inc = static_cast<float>(dx) / fsteps;
    auto y_inc = static_cast<float>(dy) / fsteps;
    auto x = static_cast<float>(begin.x);
    auto y = static_cast<float>(begin.y);
    for (auto i = 0; i <= steps; ++i) {
      plot(static_cast<unsigned>(x), static_cast<unsigned>(y));
      x += x_inc;
      y += y_inc;
    }
  }

  std::size_t render_width() const { return m_render_width; }
  std::size_t render_height() const { return m_render_height; }

private:
  std::size_t m_render_width{};
  std::size_t m_render_height{};
  std::vector<std::uint32_t> m_colors{}; // ABGR
  std::vector<float> m_depth{};
};

#endif