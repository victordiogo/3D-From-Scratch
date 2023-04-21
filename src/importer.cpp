#include "importer.hpp"

#include <SFML/Graphics/Image.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#include "timer.hpp"

auto import_texture(const std::string& texture_path) -> std::optional<Texture>
{
  auto image = sf::Image{};
  if (!image.loadFromFile(texture_path)) {
    std::cerr << "Could not load the texture " << texture_path << '\n';
    return {};
  }
  auto size = image.getSize();
  auto colors = std::vector<std::uint32_t>(size.x * size.y);
  for (auto pixel = 0U; pixel < size.x * size.y; ++pixel) {
    colors.at(pixel)
      = image.getPixel(pixel % size.x, pixel / size.x).toInteger();
  }
  return Texture{ std::move(colors), size.x, size.y };
}

// return the directory with a trailing '/'
auto get_directory(const std::string& file_path) -> std::string
{
  auto index{ file_path.find_last_of('/') };
  return (index == std::string::npos) ? "./" : file_path.substr(0, index + 1);
}

using Material = Texture;
using MaterialLib = std::map<std::string, Material>;
auto import_mtllib(const std::string& mtllib_path) -> std::optional<MaterialLib>
{
  auto mtl = std::ifstream{ mtllib_path };
  if (!mtl) {
    std::cerr << "Could not open the file " << mtllib_path << '\n';
    return {};
  }
  auto output = MaterialLib{};
  while (mtl) {
    auto line = std::string{};
    std::getline(mtl, line);
    if (line.starts_with('#') || line.length() == 0) continue;
    auto line_stream = std::stringstream{ line };
    auto head = std::string{};
    std::getline(line_stream, head, ' ');

    if (head == "newmtl") {
      auto material_name = std::string{};
      std::getline(line_stream, material_name);
      if (!line_stream) {
        std::cerr << "Could not parse the material name on line: " << line << '\n';
        return {};
      }
      while (std::getline(mtl, line)) {
        if (line.starts_with('#') || line.length() == 0) continue;
        line_stream.str(line);
        line_stream.clear();
        std::getline(line_stream, head, ' ');
        if (head == "newmtl") {
          std::cerr << "Could not find the diffuse map for material " << material_name << '\n';
          return {};
        }
        else if (head == "map_Kd") {
          auto texture_name = std::string{};
          std::getline(line_stream, texture_name);
          if (!line_stream) {
            std::cerr << "Could not parse the diffuse map name on line: " << line << '\n';
            return {};
          }
          auto texture = import_texture(get_directory(mtllib_path) + texture_name);
          if (!texture) return {};
          output.insert(std::pair{ std::move(material_name), std::move(*texture) });
          break;
        }
      }
    }
  }
  if (output.empty()) {
    std::cerr << "No materials defined on the mtl file " << mtllib_path << '\n';
    return {};
  }
  return output;
}

// Wavefront obj importer
//   accepts triangulated faces
//   accepts at least one material with a diffuse map
auto import_model(const std::string& obj_path) -> std::optional<Model>
{
  auto obj = std::ifstream{ obj_path };
  if (!obj) {
    std::cerr << "Could not open the file: " << obj_path << '\n';
    return {};
  }
  auto positions = std::vector<glm::vec3>{};
  auto texture_coords = std::vector<glm::vec2>{};
  auto output = Model{};
  auto material_lib = MaterialLib{};
  while (obj) {
    auto line = std::string{};
    std::getline(obj, line);
    if (line.starts_with('#') || line.length() == 0) continue;
    auto line_stream = std::stringstream{ line };
    auto head = std::string{};
    std::getline(line_stream, head, ' ');
    if (head == "mtllib") {
      auto material_lib_name = std::string{};
      std::getline(line_stream, material_lib_name);
      if (!line_stream) {
        std::cerr << "Could not parse the material lib name on line: " << line << '\n';
        return {};
      }
      auto optional = import_mtllib(get_directory(obj_path) + material_lib_name);
      if (!optional) return {};
      material_lib = std::move(*optional);
    }
    else if (head == "usemtl") {
      auto material_name = std::string{};
      std::getline(line_stream, material_name);
      if (!line_stream) {
        std::cerr << "Could not parse the material name on line: " << line << '\n';
        return {};
      }
      if (!material_lib.contains(material_name)) {
        std::cerr << "Could not find the material " << material_name << " in the material lib."
                  << " Occurred on the line: " << line << '\n';
        return {};
      }
      output.meshes.push_back(Mesh{ std::move(material_lib.at(material_name)) });
      material_lib.erase(material_name);
    }
    else if (head == "v") {
      auto position = glm::vec3{};
      line_stream >> position.x >> position.y >> position.z;
      if (!line_stream) {
        std::cerr << "Could not parse the geometric vertex on line: " << line << '\n';
        return {};
      }
      positions.push_back(std::move(position));
    }
    else if (head == "vt") {
      auto texture_coord = glm::vec2{};
      line_stream >> texture_coord.x >> texture_coord.y;
      if (!line_stream) {
        std::cerr << "Could not parse the texture coordinate on line: " << line << '\n';
        return {};
      }
      texture_coords.push_back(std::move(texture_coord));
    }
    else if (head == "f") {
      if (output.meshes.size() == 0) {
        std::cerr << "usemtl must be set before a face element\n";
        return {};
      }
      auto face = Mesh::Face{};
      for (auto index = 0UZ; index < 3UZ; ++index) {
        auto position_index = std::size_t{};
        line_stream >> position_index;
        line_stream.get();
        auto texture_coord_index = std::size_t{};
        line_stream >> texture_coord_index;
        if (!line_stream) {
          std::cerr << "Could not parse indices on line: " << line << '\n';
          return {};
        }
        if (position_index > positions.size() || texture_coord_index > texture_coords.size()) {
          std::cerr << "Invalid indices on line: " << line << '\n';
          return {};
        }
        line_stream.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
        face.at(index) = Vertex{ positions.at(position_index - 1), texture_coords.at(texture_coord_index - 1) };
      }
      output.meshes.back().faces.push_back(std::move(face));
    }
  }
  if (output.meshes.size() == 0) {
    std::cerr << "Could not import any model meshes on file " << obj_path << '\n';
    return {};
  }
  return output;
}