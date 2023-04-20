#ifndef _3D_FROM_SCRATCH_TEXTURE_HPP
#define _3D_FROM_SCRATCH_TEXTURE_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

class Texture final {
public:
  Texture(const std::vector<std::uint32_t>& colors, std::size_t width, std::size_t height)
    : m_colors{ colors }, m_width{ width }, m_height{ height }
  {
    if (m_colors.size() != m_width * m_height) {
      throw std::invalid_argument{ "Texture colors length must be equal to width times height" };
    }
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

#endif