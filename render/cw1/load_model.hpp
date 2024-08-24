#ifndef LOAD_MODEL_OBJ_HPP_1B67CFB6_BF91_421E_983A_CA92A246F902
#define LOAD_MODEL_OBJ_HPP_1B67CFB6_BF91_421E_983A_CA92A246F902

#include <filesystem>
#include "simple_model.hpp"
#include "../labutils/vkimage.hpp"
#include "../labutils/allocator.hpp"
#include "../labutils/vulkan_context.hpp"
#include "point_cloud.hpp"
#include "../labutils/ui.hpp"

constexpr char const* WHITE_MAT = "assets/cw1/textures/white.jpg";

//Loads file (.obj, .tri, .xyz) with config file and returns a vector of the vertex positions
std::vector<glm::vec3> load_file(std::string aPath, char const* aConfigPath, UiConfiguration& ui_config);

// Load a Wavefront OBJ model
SimpleModel load_simple_wavefront_obj( char const* aPath  );

std::vector<glm::vec3> load_triangle_soup(std::string aPath);

std::vector<glm::vec3> load_xyz(std::string aPath);

std::vector<glm::vec3> load_obj_vertices(std::string aPath);

void read_config(const std::string& filename, UiConfiguration& config);





#endif // LOAD_MODEL_OBJ_HPP_1B67CFB6_BF91_421E_983A_CA92A246F902

