#ifndef _3D_FROM_SCRATCH_IMPORTER_HPP
#define _3D_FROM_SCRATCH_IMPORTER_HPP

#include "model.hpp"

#include <fstream>
#include <map>
#include <string>

using Material = Texture;
using MaterialLib = std::map<std::string, Material>;

Model import_model(const std::string& file_path);
std::string find_mtllib_name(std::ifstream& file);
MaterialLib import_mtl(const std::string& directory, const std::string& file_path);

#endif