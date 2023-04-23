#ifndef _3D_FROM_SCRATCH_RENDERER_HPP
#define _3D_FROM_SCRATCH_RENDERER_HPP

#include "model.hpp"
#include "timer.hpp"

#include <SFML/Graphics.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

class Renderer final {
public:
  friend bool update_window(sf::RenderWindow& window, const Renderer& renderer);

  Renderer(std::size_t render_width, std::size_t render_height)
    : m_render_width{ render_width },
      m_render_height{ render_height },
      m_colors(render_width * render_height)
  {
  }

  void render(const Model& model [[maybe_unused]])
  {
    static auto s_timer = Timer{};
    // model -> world -> camera -> projection -> screen
    auto model_matrix = glm::rotate(glm::mat4{ 1.0F }, static_cast<float>(s_timer.elapsed()) / 500.0F, glm::vec3{ 0.0F, 1.0F, 0.0F });
    auto view_matrix = glm::lookAt(glm::vec3{ 0.0F, 1.5F, 5.0F }, glm::vec3{ 0.0F, 1.5F, 0.0 }, glm::vec3{ 0.0F, 1.0F, 0.0F });
    auto projection_matrix = glm::perspective(30.0F, static_cast<float>(m_render_width) / static_cast<float>(m_render_height), 0.1F, 100.0F);
    [[maybe_unused]] auto transform_matrix = projection_matrix * view_matrix * model_matrix;
    for (const auto& mesh : model.meshes) {
      for (const auto& face : mesh.faces) {
        [[maybe_unused]] auto pos_a = transform_matrix * glm::vec4{ face[0].position, 1.0F };
        pos_a /= pos_a.w;
        pos_a.x += 1.0F;
        pos_a.x *= static_cast<float>(m_render_width) * 0.5F;
        pos_a.y += 1.0F;
        pos_a.y *= static_cast<float>(m_render_height) * 0.5F;
        [[maybe_unused]] auto pos_b = transform_matrix * glm::vec4{ face[1].position, 1.0F };
        pos_b /= pos_b.w;
        pos_b.x += 1.0F;
        pos_b.x *= static_cast<float>(m_render_width) * 0.5F;
        pos_b.y += 1.0F;
        pos_b.y *= static_cast<float>(m_render_height) * 0.5F;
        [[maybe_unused]] auto pos_c = transform_matrix * glm::vec4{ face[2].position, 1.0F };
        pos_c /= pos_c.w;
        pos_c.x += 1.0F;
        pos_c.x *= static_cast<float>(m_render_width) * 0.5F;
        pos_c.y += 1.0F;
        pos_c.y *= static_cast<float>(m_render_height) * 0.5F;

        [[maybe_unused]] auto scr_a = glm::ivec2{ pos_a.x, pos_a.y };
        [[maybe_unused]] auto scr_b = glm::ivec2{ pos_b.x, pos_b.y };
        [[maybe_unused]] auto scr_c = glm::ivec2{ pos_c.x, pos_c.y };

        plot_line(scr_a, scr_b);
        plot_line(scr_b, scr_c);
        plot_line(scr_c, scr_a);
      }
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

  void plot_horizontal_line(glm::ivec2 begin, const glm::ivec2& end)
  {
    auto increment{ begin.x > end.x ? -1 : 1 };
    while (true) {
      plot(
        static_cast<std::size_t>(begin.x),
        static_cast<std::size_t>(begin.y));
      if (begin.x == end.x) break;
      begin.x += increment;
    }
  }

  void plot_line(glm::ivec2 begin, const glm::ivec2& end)
  {
    if (end.x == begin.x) plot_vertical_line(begin, end);
    if (end.y == begin.y) plot_horizontal_line(begin, end);
    auto distance_x{ std::abs(end.x - begin.x) };
    auto distance_y{ -std::abs(end.y - begin.y) };
    auto increment_x{ begin.x < end.x ? 1 : -1 };
    auto increment_y{ begin.y < end.y ? 1 : -1 };
    auto error{ distance_x + distance_y };
    // auto t{ 0.0 }; // interpolation parameter
    // auto t_increment{ 1.0 / std::sqrt(distance_x * distance_x + distance_y * distance_y) };
    while (true) {
      plot(
        static_cast<std::size_t>(begin.x),
        static_cast<std::size_t>(begin.y));
      if (begin.x == end.x && begin.y == end.y) break;
      auto error2{ 2 * error };
      if (error2 >= distance_y) {
        if (begin.x == end.x) break;
        error += distance_y;
        begin.x += increment_x;
      }
      if (error2 <= distance_x) {
        if (begin.y == end.y) break;
        error += distance_x;
        begin.y += increment_y;
      }
      // t += t_increment;
    }
  }

  std::size_t render_width() const { return m_render_width; }
  std::size_t render_height() const { return m_render_height; }

private:
  std::size_t m_render_width{};
  std::size_t m_render_height{};
  std::vector<std::uint32_t> m_colors{};
};

#endif