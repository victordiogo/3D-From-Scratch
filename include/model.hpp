#ifndef _3D_FROM_SCRATCH_MODEL_HPP
#define _3D_FROM_SCRATCH_MODEL_HPP

#include "texture.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct Vertex {
  glm::vec3 position{};
  glm::vec2 texture_coord{};
};

struct Mesh {
  using Face = std::array<Vertex, 3>;
  Texture texture;
  std::vector<Face> faces{};
};

struct Model {
  std::vector<Mesh> meshes{};
};

#endif