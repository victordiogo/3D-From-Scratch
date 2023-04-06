#include "graphics.hpp"

#include <bitset>
#include <glm/vec2.hpp>
int main()
{
  Graphics graphics{ 800, 600 };
  graphics.plot_line(glm::ivec2{ 0, 1 }, glm::ivec2{ 500, 100 });
  graphics.save_image();

  return 0;
}