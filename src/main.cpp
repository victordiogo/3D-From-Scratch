#include "importer.hpp"
#include "model.hpp"
#include "renderer.hpp"
#include "timer.hpp"

#include <SFML/Graphics.hpp>
#include <glm/vec2.hpp>

#include <stdexcept>

bool update_window(sf::RenderWindow& window, const Renderer& renderer)
{
  auto texture = sf::Texture{};
  if (!texture.create(
        static_cast<unsigned>(renderer.m_render_width),
        static_cast<unsigned>(renderer.m_render_height))) {
    return false;
  }
  texture.update(
    reinterpret_cast<const std::uint8_t*>(renderer.m_colors.data()));
  window.draw(sf::Sprite{ texture });
  return true;
}

// to do: "if"s with more than 1 line must have brackets
// analyze reinterpret_cast alternatives
auto main() -> int
{
  Timer timer{};
  auto model = import_model("C:/Users/Victor/Documents/3d_models/Artorias/model.obj");
  std::cout << timer.elapsed() << '\n';
  if (!model) return 1;

  constexpr auto width = 1280;
  constexpr auto height = 720;
  auto renderer = Renderer{ width, height };
  auto window = sf::RenderWindow{ sf::VideoMode{ width, height }, "" };

  for (auto timer = Timer{}; window.isOpen(); timer.reset()) {
    auto event = sf::Event{};
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    renderer.plot_line(glm::ivec2{ 0, 0 }, glm::ivec2{ 500, 100 });
    update_window(window, renderer);
    window.display();
    window.setTitle("Render time: " + std::to_string(timer.elapsed()) + "ms");
  }

  return 0;
}
