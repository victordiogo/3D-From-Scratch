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

struct ClipVertex {
  glm::vec4 position{};
  glm::vec2 texture_coord{};
};

struct ScreenVertex {
  float inv_z{};
  glm::ivec2 position{};
  glm::vec2 texture_coord{};
};

inline auto to_screen_vertex(const ClipVertex& v, int width, int height) -> ScreenVertex
{
  assert(v.position.w != 0.0F);
  auto norm_x = v.position.x / v.position.w;
  auto norm_y = v.position.y / v.position.w;
  auto norm_z = v.position.z / v.position.w;
  return {
    1.0F / norm_z,
    glm::ivec2{
      glm::clamp(static_cast<int>((norm_x * 0.5F + 0.5F) * static_cast<float>(width)), 0, width - 1),
      glm::clamp(static_cast<int>((norm_y * 0.5F + 0.5F) * static_cast<float>(height)), 0, height - 1) },
    v.texture_coord
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
      m_colors(render_width * render_height)
  {
  }

  void render(const Model& model)
  {
    static auto s_timer = Timer{};
    auto model_matrix = glm::rotate(glm::mat4{ 1.0F }, static_cast<float>(s_timer.elapsed()) / 3000.0F, glm::vec3{ 0.0F, 1.0F, 0.0F });
    model_matrix = glm::translate(model_matrix, { 6 * glm::sin(s_timer.elapsed() / 2000.0), 0, 0 });
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
            ClipVertex{ vert_c, face[2].texture_coord });
        }
        else {
          auto clipped_verts = clip_triangle(
            ClipVertex{ vert_a, face[0].texture_coord },
            ClipVertex{ vert_b, face[1].texture_coord },
            ClipVertex{ vert_c, face[2].texture_coord });
          for (auto index = 2UZ; index < clipped_verts.size(); ++index) {
            render_triangle(clipped_verts[0], clipped_verts[index - 1], clipped_verts[index]);
          }
        }
      }
    }
  }

  void render_triangle(const ClipVertex& clip_vert_a, const ClipVertex& clip_vert_b, const ClipVertex& clip_vert_c)
  {
    auto width = static_cast<int>(m_render_width);
    auto height = static_cast<int>(m_render_height);
    auto vert_a = to_screen_vertex(clip_vert_a, width, height);
    auto vert_b = to_screen_vertex(clip_vert_b, width, height);
    auto vert_c = to_screen_vertex(clip_vert_c, width, height);
    order_vertices_ascending_y(vert_a, vert_b, vert_c);

    if (vert_c.position.y == vert_a.position.y) {
      // draw horizontal line
      return;
    }

    if (vert_a.position.y == vert_b.position.y) {
      render_top_flat_triangle(vert_a, vert_b, vert_c);
    }
    else if (vert_b.position.y == vert_c.position.y) {
      render_bottom_flat_triangle(vert_a, vert_b, vert_c);
    }
    else {
      auto t = static_cast<float>(vert_b.position.y - vert_a.position.y) / static_cast<float>(vert_c.position.y - vert_a.position.y);
      auto v = ScreenVertex{
        vert_b.inv_z,
        glm::ivec2{ static_cast<float>(vert_a.position.x) + t * static_cast<float>(vert_b.position.x - vert_a.position.x), vert_b.position.y },
        vert_b.texture_coord
      };
      render_bottom_flat_triangle(vert_a, vert_b, v);
      render_top_flat_triangle(vert_b, v, vert_c);
    }
  }

  void render_bottom_flat_triangle(const ScreenVertex& top, const ScreenVertex& bottom_a, const ScreenVertex& bottom_b)
  {
    auto topa_dist_x{ std::abs(bottom_a.position.x - top.position.x) };
    auto topa_dist_y{ top.position.y - bottom_a.position.y };
    auto topa_inc_x{ top.position.x < bottom_a.position.x ? 1 : -1 };
    auto topa_error{ topa_dist_x + topa_dist_y };
    auto topa_x = top.position.x;

    auto topb_dist_x{ std::abs(bottom_b.position.x - top.position.x) };
    auto topb_dist_y{ top.position.y - bottom_b.position.y };
    auto topb_inc_x{ top.position.x < bottom_b.position.x ? 1 : -1 };
    auto topb_error{ topb_dist_x + topb_dist_y };
    auto topb_x = top.position.x;

    auto y = top.position.y;

    while (true) {
      plot_horizontal_line(
        topa_x,
        topb_x,
        y);
      if (topa_x == bottom_a.position.x && topb_x == bottom_b.position.x && y == bottom_a.position.y) break;
      auto topa_error2{ 2 * topa_error };
      if (topa_error2 >= topa_dist_y) {
        if (topa_x == bottom_a.position.x) break;
        topa_error += topa_dist_y;
        topa_x += topa_inc_x;
      }
      if (topa_error2 <= topa_dist_x) {
        if (y == bottom_a.position.y) break;
        topa_error += topa_dist_x;
        topb_error += topb_dist_x;
        ++y;
      }
      auto topb_error2{ 2 * topb_error };
      if (topb_error2 >= topb_dist_y) {
        if (topb_x == bottom_b.position.x) break;
        topb_error += topb_dist_y;
        topb_x += topb_inc_x;
      }
    }
  }

  void render_top_flat_triangle(const ScreenVertex& top_a, const ScreenVertex& top_b, const ScreenVertex& bottom)
  {
    auto abottom_dist_x{ std::abs(bottom.position.x - top_a.position.x) };
    auto abottom_dist_y{ top_a.position.y - bottom.position.y };
    auto abottom_inc_x{ top_a.position.x < bottom.position.x ? 1 : -1 };
    auto abottom_error{ abottom_dist_x + abottom_dist_y };
    auto abottom_x = top_a.position.x;

    auto bbottom_dist_x{ std::abs(bottom.position.x - top_b.position.x) };
    auto bbottom_dist_y{ top_b.position.y - bottom.position.y };
    auto bbottom_inc_x{ top_b.position.x < bottom.position.x ? 1 : -1 };
    auto bbottom_error{ bbottom_dist_x + bbottom_dist_y };
    auto bbottom_x = top_b.position.x;

    auto y = top_a.position.y;

    while (true) {
      plot_horizontal_line(
        abottom_x,
        bbottom_x,
        y);
      if (abottom_x == bottom.position.x && bbottom_x == bottom.position.x && y == bottom.position.y) break;
      auto abottom_error2{ 2 * abottom_error };
      if (abottom_error2 >= abottom_dist_y) {
        if (abottom_x == bottom.position.x) break;
        abottom_error += abottom_dist_y;
        abottom_x += abottom_inc_x;
      }
      if (abottom_error2 <= abottom_dist_x) {
        if (y == bottom.position.y) break;
        abottom_error += abottom_dist_x;
        bbottom_error += bbottom_dist_x;
        ++y;
      }
      auto bbottom_error2{ 2 * bbottom_error };
      if (bbottom_error2 >= bbottom_dist_y) {
        if (bbottom_x == bottom.position.x) break;
        bbottom_error += bbottom_dist_y;
        bbottom_x += bbottom_inc_x;
      }
    }
  }

  void order_vertices_ascending_y(ScreenVertex& vert_a, ScreenVertex& vert_b, ScreenVertex& vert_c)
  {
    if (vert_b.position.y < vert_a.position.y) {
      std::swap(vert_b, vert_a);
    }
    if (vert_c.position.y < vert_b.position.y) {
      std::swap(vert_b, vert_c);
    }
    if (vert_b.position.y < vert_a.position.y) {
      std::swap(vert_b, vert_a);
    }
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

  void plot_horizontal_line(int beg_x, int end_x, int y)
  {
    auto increment{ beg_x > end_x ? -1 : 1 };
    while (true) {
      plot(
        static_cast<std::size_t>(beg_x),
        static_cast<std::size_t>(y));
      if (beg_x == end_x) break;
      beg_x += increment;
    }
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

  // void plot_line(glm::ivec2 begin, const glm::ivec2& end)
  // {
  //   if (end.x == begin.x) plot_vertical_line(begin, end);
  //   if (end.y == begin.y) plot_horizontal_line(begin, end);
  //   auto distance_x{ std::abs(end.x - begin.x) };
  //   auto distance_y{ -std::abs(end.y - begin.y) };
  //   auto increment_x{ begin.x < end.x ? 1 : -1 };
  //   auto increment_y{ begin.y < end.y ? 1 : -1 };
  //   auto error{ distance_x + distance_y };
  //   // auto t{ 0.0 }; // interpolation parameter
  //   // auto t_increment{ 1.0 / std::sqrt(distance_x * distance_x + distance_y * distance_y) };
  //   while (true) {
  //     plot(
  //       static_cast<std::size_t>(begin.x),
  //       static_cast<std::size_t>(begin.y));
  //     if (begin.x == end.x && begin.y == end.y) break;
  //     auto error2{ 2 * error };
  //     if (error2 >= distance_y) {
  //       if (begin.x == end.x) break;
  //       error += distance_y;
  //       begin.x += increment_x;
  //     }
  //     if (error2 <= distance_x) {
  //       if (begin.y == end.y) break;
  //       error += distance_x;
  //       begin.y += increment_y;
  //     }
  //     // t += t_increment;
  //   }
  // }

  std::size_t render_width() const { return m_render_width; }
  std::size_t render_height() const { return m_render_height; }

private:
  std::size_t m_render_width{};
  std::size_t m_render_height{};
  std::vector<std::uint32_t> m_colors{};
};

#endif