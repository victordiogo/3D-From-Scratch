#include "clipper.hpp"

#include <glm/gtx/compatibility.hpp>

// Param: vertice in clip space
auto is_vertice_inside_frustum(const glm::vec4& v) -> bool
{
  // after w division, to be inside the frustum, each normalized coordinate c in (x, y, z) should be ruled for: -1 <= c <= 1
  return -v.w <= v.x && v.x <= v.w
         && -v.w <= v.y && v.y <= v.w
         && -v.w <= v.z && v.z <= v.w;
}

// Param: vertice in clip space
auto is_vertice_outside_frustum(const glm::vec4& v) -> bool
{
  return !is_vertice_inside_frustum(v);
}

// Params: vertices in clip space
auto is_triangle_outside_frustum(const glm::vec4& vert_a, const glm::vec4& vert_b, const glm::vec4& vert_c) -> bool
{
  return is_vertice_outside_frustum(vert_a)
         && is_vertice_outside_frustum(vert_b)
         && is_vertice_outside_frustum(vert_c);
}

// Params: vertices in clip space
auto is_triangle_inside_frustum(const glm::vec4& vert_a, const glm::vec4& vert_b, const glm::vec4& vert_c) -> bool
{
  return is_vertice_inside_frustum(vert_a)
         && is_vertice_inside_frustum(vert_b)
         && is_vertice_inside_frustum(vert_c);
}

// Param: vertice in clip space
auto dot_product(const glm::vec4& v, FrustumPlane fplane) -> float
{
  switch (fplane) {
    case near:
      return v.z + v.w;
    case far:
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

// it needs to be an outer bound clipper, because current rasterizer
// is clipping against screen boundaries
// I'll need to know the precision of fixed point variables used in the rasterizer
auto clip_triangle(const ClipVertex& vert_a, const ClipVertex& vert_b, const ClipVertex& vert_c) -> std::vector<ClipVertex>
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