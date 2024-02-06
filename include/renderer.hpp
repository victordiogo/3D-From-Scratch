#ifndef _3D_FROM_SCRATCH_RENDERER_HPP
#define _3D_FROM_SCRATCH_RENDERER_HPP

#include "clipper.hpp"
#include "scene.hpp"
#include "texture.hpp"

#include <glm/vec4.hpp>

#include <cstddef>
#include <cstdint>
#include <numeric>

class Renderer final {
public:
  Renderer(std::size_t render_width, std::size_t render_height)
    : m_render_width{ render_width },
      m_render_height{ render_height },
      m_colors(render_width * render_height),
      m_depth(render_width * render_height)
  {
  }

  void clear()
  {
    static auto s_background = std::vector<std::uint32_t>(m_render_width * m_render_height, 0xFF000000);
    m_colors = s_background;
    static auto s_depth = std::vector<float>(m_render_width * m_render_height, std::numeric_limits<float>::max());
    m_depth = s_depth;
  }

  void plot(std::size_t x, std::size_t y, std::uint32_t color)
  {
    assert(x < m_render_width && y < m_render_height);
    m_colors[y * m_render_width + x] = color;
  }

  void render(const Scene& scene);
  void render_triangle(const ClipVertex& ca, const ClipVertex& cb, const ClipVertex& cc, const Texture& texture);
  auto get_screen_position(const glm::vec4& v) const -> glm::vec2;
  auto render_width() const -> std::size_t { return m_render_width; }
  auto render_height() const -> std::size_t { return m_render_height; }
  auto colors() const -> const std::vector<std::uint32_t>& { return m_colors; }

private:
  std::size_t m_render_width{};
  std::size_t m_render_height{};
  std::vector<std::uint32_t> m_colors{};
  std::vector<float> m_depth{};
};

#endif