#include "importer.hpp"

#include <fstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using Tokens = std::vector<std::string>;
Tokens split(std::string_view str, char delimiter)
{
  Tokens tokens{};
  std::string token{ "" };
  for (; str.length() > 0; str.remove_prefix(1)) {
    if (str.at(0) == delimiter) {
      tokens.push_back(std::move(token));
      token = "";
    }
    else {
      token += str.at(0);
    }
  }
  return tokens;
}

// return the directory with a trailing '/'
std::string get_directory(const std::string& file_path)
{
  auto index{ file_path.find_last_of('/') };
  return (index == std::string::npos) ? "./" : file_path.substr(0, index + 1);
}

Model import_model(const std::string& file_path)
{
  std::ifstream file{ file_path };
  if (!file)
    throw std::invalid_argument{ "Could not open file" };
  std::string mtllib_name{ find_mtllib_name(file) };
  std::string directory{ get_directory(file_path) };
  MaterialLib material_lib{ import_mtl(directory, directory + mtllib_name) };
  file.seekg(std::ios::beg);
  while (file) {
    std::string line{};
    std::getline(file, line);
    if (line.length() == 0) continue;
    Tokens tokens{ split(line, ' ') };
    std::string& element{ tokens.at(0) };
    if (element == "v") {
      if (tokens.size() != 4)
        throw std::invalid_argument{ "Invalid geometric vertex" };
      // get_geometric_vertex:
      // glm::vec3{ std::stof(tokens.at(1)), std::stof(tokens.at(2)), std::stof(tokens.at(3)) }
    }
    if (element == "vt") {
      if (tokens.size() != 3)
        throw std::invalid_argument{ "Invalid texture coordinate" };
      // get_texture_coordinate:
      // glm::vec2{ std::stof(tokens.at(1)), std::stof(tokens.at(2)) }
    }
    if (element == "f") {
      if (tokens.size() != 4)
        throw std::invalid_argument{ "Invalid face" };
      // get_face_indices
      // bind Vertex
    }
  }
  return {};
}

std::string find_mtllib_name(std::ifstream& file)
{
  while (file) {
    std::string line{};
    std::getline(file, line);
    if (line.starts_with("mtllib")) {
      auto space_index{ line.find(' ') };
      if (space_index == std::string::npos)
        throw std::invalid_argument{ "Invalid mtllib file name" };
      return line.substr(space_index + 1);
    }
  }
  throw std::invalid_argument{ "Could not find mtllib" };
}

MaterialLib import_mtl(const std::string& directory, const std::string& file_path)
{
  std::ifstream file{ file_path };
  if (!file)
    throw std::invalid_argument{ "Could not open file" };
  MaterialLib material_lib{};
  return material_lib;
}