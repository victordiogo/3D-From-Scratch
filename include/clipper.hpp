#ifndef _3D_FROM_SCRATCH_CLIPPING_HPP
#define _3D_FROM_SCRATCH_CLIPPING_HPP

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <vector>

auto is_vertice_inside_frustum(const glm::vec4& v) -> bool;
auto is_vertice_outside_frustum(const glm::vec4& v) -> bool;
auto is_triangle_outside_frustum(const glm::vec4& vert_a, const glm::vec4& vert_b, const glm::vec4& vert_c) -> bool;
auto is_triangle_inside_frustum(const glm::vec4& vert_a, const glm::vec4& vert_b, const glm::vec4& vert_c) -> bool;
enum FrustumPlane {
  near,
  far,
  left,
  bottom,
  right,
  top,
  plane_count
};
auto dot_product(const glm::vec4& v, FrustumPlane fplane) -> float;
struct ClipVertex {
  glm::vec4 position{};
  glm::vec2 texture_coord{};
};
auto clip_triangle(const ClipVertex& vert_a, const ClipVertex& vert_b, const ClipVertex& vert_c) -> std::vector<ClipVertex>;

#endif