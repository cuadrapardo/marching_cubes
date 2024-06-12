#ifndef LOAD_MODEL_OBJ_HPP_1B67CFB6_BF91_421E_983A_CA92A246F902
#define LOAD_MODEL_OBJ_HPP_1B67CFB6_BF91_421E_983A_CA92A246F902

#include <filesystem>
#include "simple_model.hpp"
#include "../labutils/vkimage.hpp"
#include "../labutils/allocator.hpp"
#include "../labutils/vulkan_context.hpp"
#include "point_cloud.hpp"

constexpr char const* WHITE_MAT = "assets/cw1/textures/white.jpg";

PointCloud load_file(char const* aPath, labutils::VulkanContext const& window, labutils::Allocator const& allocator);

// Load a Wavefront OBJ model
SimpleModel load_simple_wavefront_obj( char const* aPath );

std::vector<glm::vec3> load_triangle_soup(char const* aPath);



#endif // LOAD_MODEL_OBJ_HPP_1B67CFB6_BF91_421E_983A_CA92A246F902

