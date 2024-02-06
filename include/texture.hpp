#ifndef _3D_FROM_SCRATCH_TEXTURE_HPP
#define _3D_FROM_SCRATCH_TEXTURE_HPP

#include <algorithm>
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

  auto operator[](std::size_t x, std::size_t y) -> std::uint32_t&
  {
    x = std::clamp(x, 0UZ, m_width - 1);
    y = std::clamp(y, 0UZ, m_height - 1);
    return m_colors[y * m_width + x];
  }

  auto operator[](std::size_t x, std::size_t y) const -> std::uint32_t
  {
    x = std::clamp(x, 0UZ, m_width - 1);
    y = std::clamp(y, 0UZ, m_height - 1);
    return m_colors[y * m_width + x];
  }

  auto width() const -> std::size_t { return m_width; }
  auto height() const -> std::size_t { return m_height; }

private:
  std::vector<std::uint32_t> m_colors{};
  std::size_t m_width{};
  std::size_t m_height{};
};

#endif