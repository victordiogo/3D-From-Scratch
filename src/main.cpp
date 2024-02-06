#include "importer.hpp"
#include "model.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "timer.hpp"

#include <SFML/Graphics.hpp>
#include <glm/vec2.hpp>

#include <cassert>
#include <stdexcept>

auto draw_window(sf::RenderWindow& window, const Renderer& renderer) -> bool
{
  auto texture = sf::Texture{};
  if (!texture.create(
        static_cast<unsigned>(renderer.render_width()),
        static_cast<unsigned>(renderer.render_height()))) {
    assert(false && "Could not create texture");
    return false;
  }
  texture.update(
    reinterpret_cast<const std::uint8_t*>(renderer.colors().data()));
  window.draw(sf::Sprite{ texture });
  return true;
}

auto main() -> int
{
  // auto model_path = std::string{};
  // std::cout << "Enter the model obj file path (with \"/\" delimiters): ";
  // std::getline(std::cin >> std::ws, model_path);
  // auto model = import_model(model_path);
  auto model = import_model("C:/Users/Victor/Documents/3d_models/Skull/model.obj");
  if (!model) return 1;
  model->position.z = 0;

  auto scene = Scene{ *model };

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

    renderer.clear();
    renderer.render(scene);
    draw_window(window, renderer);
    window.display();
    window.setTitle("Render time: " + std::to_string(timer.elapsed()) + "ms");

    model->rotation.y += static_cast<float>(timer.elapsed() / 1000.0);
  }

  return 0;
}
