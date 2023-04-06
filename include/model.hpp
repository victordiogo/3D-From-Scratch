#ifndef _3D_FROM_SCRATCH_MODEL_HPP
#define _3D_FROM_SCRATCH_MODEL_HPP

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

class Texture final {
public:
  Texture(const std::vector<std::uint32_t>& colors, std::size_t width, std::size_t height)
    : m_colors{ colors }, m_width{ width }, m_height{ height }
  {
    assert(m_colors.size() == m_width * m_height);
  }

  std::uint32_t& operator[](std::size_t x, std::size_t y)
  {
    assert(x < m_width && y < m_height);
    return m_colors[y * m_width + x];
  }

  std::uint32_t operator[](std::size_t x, std::size_t y) const
  {
    assert(x < m_width && y < m_height);
    return m_colors[y * m_width + x];
  }

private:
  std::vector<std::uint32_t> m_colors{};
  std::size_t m_width{};
  std::size_t m_height{};
};

struct Vertex {
  glm::vec3& position{};
  glm::vec2& texture_coord{};
};

using Triangle = std::array<Vertex, 3>;

struct Mesh {
  std::vector<Triangle> faces{};
  Texture& texture{};
};

class Model final {
private:
  std::vector<Mesh> m_meshes{};
  std::vector<glm::vec3> m_positions{};
  std::vector<glm::vec2> m_texture_coords{};
  std::vector<Texture> m_textures{};
};

#endif