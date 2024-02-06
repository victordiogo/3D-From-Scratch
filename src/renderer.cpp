#include "renderer.hpp"
#include "clipper.hpp"
#include "model.hpp"
#include "timer.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <cstdint>

#include <iostream>

auto get_model_matrix(const Model& model) -> glm::mat4
{
  auto transform = glm::translate(glm::mat4{ 1.0F }, model.position);
  transform = glm::rotate(transform, model.rotation.x, glm::vec3{ 1.0F, 0.0F, 0.0F });
  transform = glm::rotate(transform, model.rotation.y, glm::vec3{ 0.0F, 1.0F, 0.0F });
  transform = glm::rotate(transform, model.rotation.z, glm::vec3{ 0.0F, 0.0F, 1.0F });
  transform = glm::scale(transform, model.scale);
  return transform;
}

void Renderer::render(const Scene& scene)
{
  auto view_matrix = glm::lookAt(glm::vec3{ 0.0F, 1.5F, 8.0F }, glm::vec3{ 0.0F, 1.5F, 0.0 }, glm::vec3{ 0.0F, 1.0F, 0.0F });
  auto projection_matrix = glm::perspective(
    glm::radians(60.0F),
    static_cast<float>(m_render_width) / static_cast<float>(m_render_height),
    0.1F,
    100.0F);
  auto transform_matrix = projection_matrix * view_matrix * get_model_matrix(scene.model);
  for (const auto& mesh : scene.model.meshes) {
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

constexpr auto g_num_fractional_bits = 18;
using Fixed = std::int32_t;
auto to_fixed(float num) -> Fixed
{
  return static_cast<Fixed>(num * (1 << g_num_fractional_bits) + 0.5F);
}

auto from_fixed(Fixed num) -> float
{
  return static_cast<float>(num) / (1 << g_num_fractional_bits);
}

// It verifies if a pixel center lying exactly on an edge needs to be rendered.
// The objective is to avoid rendering the same edge two times in case of overlaping triangles
bool needs_to_render_edge(const glm::ivec2& edge)
{
  // Horizontal edge
  if (edge.y == 0) {
    return edge.x < 0;
  }
  // Vertical edge
  else if (edge.x == 0) {
    return edge.y > 0;
  }
  // Diagonal edge
  else {
    return edge.x > 0;
  }
}

auto cross(const glm::vec2& a, const glm::vec2& b) -> float
{
  return a.x * b.y - a.y * b.x;
}

auto Renderer::get_screen_position(const glm::vec4& v) const -> glm::vec2
{
  auto max_x = static_cast<float>(m_render_width - 1);
  auto x = ((v.x / v.w) * 0.5F + 0.5F) * max_x;
  auto max_y = static_cast<float>(m_render_height - 1);
  auto y = ((-v.y / v.w) * 0.5F + 0.5F) * max_y;
  return { x, y };
}

// accepts counter-clockwise triangles
void Renderer::render_triangle(const ClipVertex& p, const ClipVertex& q, const ClipVertex& r, const Texture& texture)
{
  auto screen_a = get_screen_position(p.position);
  auto screen_b = get_screen_position(q.position);
  auto screen_c = get_screen_position(r.position);

  auto inv_z_a = p.position.w / p.position.z;
  auto inv_z_b = q.position.w / q.position.z;
  auto inv_z_c = r.position.w / r.position.z;

  auto tcoord_a = p.texture_coord * inv_z_a;
  auto tcoord_b = q.texture_coord * inv_z_b;
  auto tcoord_c = r.texture_coord * inv_z_c;

  auto xmin = static_cast<int>(std::min(std::min(screen_a.x, screen_b.x), screen_c.x) + 0.5F);
  xmin = glm::clamp(xmin, 0, static_cast<int>(m_render_width) - 1);
  auto xmax = static_cast<int>(std::max(std::max(screen_a.x, screen_b.x), screen_c.x) - 0.5F);
  xmax = glm::clamp(xmax, 0, static_cast<int>(m_render_width) - 1);

  auto ymin = static_cast<int>(std::min(std::min(screen_a.y, screen_b.y), screen_c.y) + 0.5F);
  ymin = glm::clamp(ymin, 0, static_cast<int>(m_render_height) - 1);
  auto ymax = static_cast<int>(std::max(std::max(screen_a.y, screen_b.y), screen_c.y) - 0.5F);
  ymax = glm::clamp(ymax, 0, static_cast<int>(m_render_height) - 1);

  auto start = glm::vec2{
    static_cast<float>(xmin) + 0.5F,
    static_cast<float>(ymin) + 0.5F
  };

  auto cb = screen_c - screen_b;
  auto ac = screen_a - screen_c;
  auto ba = screen_b - screen_a;

  auto bias_a = needs_to_render_edge(cb) ? 0 : -1;
  auto bias_b = needs_to_render_edge(ac) ? 0 : -1;
  auto bias_c = needs_to_render_edge(ba) ? 0 : -1;

  auto wa = to_fixed(cross(start - screen_b, cb)) + bias_a;
  auto wb = to_fixed(cross(start - screen_c, ac)) + bias_b;
  auto wc = to_fixed(cross(start - screen_a, ba)) + bias_c;

  auto wa_xinc = to_fixed(cb.y);
  auto wb_xinc = to_fixed(ac.y);
  auto wc_xinc = to_fixed(ba.y);

  auto wa_yinc = to_fixed(-cb.x);
  auto wb_yinc = to_fixed(-ac.x);
  auto wc_yinc = to_fixed(-ba.x);

  auto area = cross(-ba, cb);
  if (area <= 0.0F) return;
  auto inv_area = 1.0F / area;

  for (auto y = ymin; y <= ymax; ++y) {
    auto wa_x = wa;
    auto wb_x = wb;
    auto wc_x = wc;

    for (auto x = xmin; x <= xmax; ++x) {
      if (wa_x >= 0 && wb_x >= 0 && wc_x >= 0) {
        assert(x < m_render_width && y < m_render_height);
        auto alpha = from_fixed(wa_x) * inv_area;
        auto beta = from_fixed(wb_x) * inv_area;
        auto gama = from_fixed(wc_x) * inv_area;
        auto z = 1.0F / (alpha * inv_z_a + beta * inv_z_b + gama * inv_z_c);
        auto screen_index = static_cast<std::size_t>(y) * m_render_width
                            + static_cast<std::size_t>(x);
        if (z < m_depth[screen_index]) {
          m_depth[screen_index] = z;
          auto tcoord = z * (alpha * tcoord_a + beta * tcoord_b + gama * tcoord_c);
          m_colors[screen_index]
            = texture[static_cast<std::size_t>(tcoord.x), static_cast<std::size_t>(tcoord.y)];
        }
      }
      wa_x += wa_xinc;
      wb_x += wb_xinc;
      wc_x += wc_xinc;
    }
    wa += wa_yinc;
    wb += wb_yinc;
    wc += wc_yinc;
  }
}