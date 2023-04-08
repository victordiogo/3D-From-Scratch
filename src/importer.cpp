#include "importer.hpp"

#include <array>
#include <cassert>
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

std::string get_material_name(const std::string& line)
{
  auto space_index{ line.find(' ') };
  if (space_index == std::string::npos)
    throw std::invalid_argument{ "Could not get material name" };
  return line.substr(space_index + 1);
}

glm::vec3 get_geometric_vertex(const Tokens& tokens)
{
  if (tokens.size() != 4)
    throw std::invalid_argument{ "Invalid geometric vertex" };
  return { std::stof(tokens.at(1)), std::stof(tokens.at(2)), std::stof(tokens.at(3)) };
}

glm::vec2 get_texture_coordinate(const Tokens& tokens)
{
  if (tokens.size() != 3)
    throw std::invalid_argument{ "Invalid texture coordinate" };
  return { std::stof(tokens.at(1)), std::stof(tokens.at(2)) };
}

struct VertexIndices {
  std::size_t position{};
  std::size_t texture_coord{};
};

VertexIndices get_vertex_indices(std::string_view vertex)
{
  auto tokens{ split(vertex, '/') };
  if (tokens.size() < 2 || tokens.size() > 3)
    throw std::invalid_argument{ "Invalid number of face indices" };
  return {
    std::stoul(tokens.at(0)) - 1,
    std::stoul(tokens.at(1)) - 1
  };
}

using Face = std::array<VertexIndices, 3>;
Face get_face(const Tokens& tokens)
{
  if (tokens.size() != 4)
    throw std::invalid_argument{ "Invalid face" };
  auto first{ get_vertex_indices(tokens.at(1)) };
  auto second{ get_vertex_indices(tokens.at(2)) };
  auto third{ get_vertex_indices(tokens.at(3)) };
  return { first, second, third };
}

// Wavefront obj importer
Model import_model(const std::string& file_path)
{
  std::ifstream file{ file_path };
  if (!file)
    throw std::invalid_argument{ "Could not open obj file" };
  MaterialLib material_lib{ import_mtl(file, get_directory(file_path)) };
  Model output{};
  while (file) {
    std::string line{};
    std::getline(file, line);
    if (line.length() == 0) continue;
    Tokens tokens{ split(line, ' ') };
    if (tokens.at(0) == "usemtl") {
      auto material_name{ get_material_name(line) };
      // create mesh with a bound texture
    }
    if (tokens.at(0) == "v") {
      auto position{ get_geometric_vertex(tokens) };
      output.m_positions.push_back(position);
    }
    if (tokens.at(0) == "vt") {
      auto texture_coord{ get_texture_coordinate(tokens) };
      output.m_texture_coords.push_back(texture_coord);
    }
    if (tokens.at(0) == "f") {
      if (output.m_meshes.size() == 0)
        throw std::invalid_argument{ "usemtl must be set before a face element" };
      auto face{ get_face(tokens) };
      try {
        Vertex v1{ output.m_positions.at(face[0].position), output.m_texture_coords.at(face[0].texture_coord) };
        Vertex v2{ output.m_positions.at(face[1].position), output.m_texture_coords.at(face[1].texture_coord) };
        Vertex v3{ output.m_positions.at(face[2].position), output.m_texture_coords.at(face[2].texture_coord) };
        output.m_meshes.back().faces.push_back({ v1, v2, v3 });
      }
      catch (const std::out_of_range& e) {
        throw std::invalid_argument{ "Invalid face indices" };
      }
    }
  }
  return output;
}

std::string find_mtllib_name(std::ifstream& file)
{
  while (file) {
    std::string line{};
    std::getline(file, line);
    if (line.starts_with("mtllib")) {
      auto space_index{ line.find(' ') };
      if (space_index == std::string::npos)
        throw std::invalid_argument{ "Could not get mtllib file name" };
      return line.substr(space_index + 1);
    }
  }
  throw std::invalid_argument{ "Could not find mtllib" };
}

MaterialLib import_mtl(std::ifstream& obj_file, const std::string& directory)
{
  std::string mtllib_name{ find_mtllib_name(obj_file) };
  obj_file.seekg(std::ios::beg);
  std::ifstream mtl_file{ directory + mtllib_name };
  if (!mtl_file)
    throw std::invalid_argument{ "Could not open mtl file" };
  MaterialLib material_lib{};
  return material_lib;
}