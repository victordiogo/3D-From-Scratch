#ifndef _3D_FROM_SCRATCH_GRAPHICS_HPP
#define _3D_FROM_SCRATCH_GRAPHICS_HPP

#include <glm/gtx/compatibility.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

class Graphics final {
public:
  Graphics(std::size_t screen_width, std::size_t screen_height)
    : m_screen_width{ screen_width },
      m_screen_height{ screen_height },
      m_screen(screen_width * screen_height)
  {
  }

  void plot(std::size_t x, std::size_t y, std::uint32_t color)
  {
    m_screen[y * m_screen_width + x] = color;
  }

  void plot_vertical_line(glm::ivec2 begin, const glm::ivec2& end)
  {
    auto increment{ begin.y > end.y ? -1 : 1 };
    while (true) {
      plot(
        static_cast<std::size_t>(begin.x),
        static_cast<std::size_t>(begin.y),
        0xFF00FF00);
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
        static_cast<std::size_t>(begin.y),
        0xFF00FF00);
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
    // auto t{ 0.0 }; // interpolation coefficient
    // auto t_increment{ 1.0 / std::sqrt(distance_x * distance_x + distance_y * distance_y) };
    while (true) {
      plot(
        static_cast<std::size_t>(begin.x),
        static_cast<std::size_t>(begin.y),
        0xFF00FF00);
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

  void save_image()
  {
    std::ofstream file{ "output.ppm", std::ios::binary };
    file << "P6\n"
         << m_screen_width << ' ' << m_screen_height << '\n'
         << "255\n";
    for (const auto& color : m_screen) {
      file << static_cast<char>(color >> 24)
           << static_cast<char>((color & 0x00FF0000) >> 16)
           << static_cast<char>((color & 0x0000FF00) >> 8);
    }
  }

private:
  std::size_t m_screen_width{};
  std::size_t m_screen_height{};
  std::vector<std::uint32_t> m_screen{};
};

#endif