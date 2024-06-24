//
// Created by Carolina Cuadra Pardo on 6/17/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_DISTANCE_FIELD_HPP
#define MARCHING_CUBES_POINT_CLOUD_DISTANCE_FIELD_HPP

#include <glm/vec3.hpp>
#include <vector>

#include "../render/labutils/vulkan_context.hpp"
#include "../render/labutils/allocator.hpp"
#include "../render/labutils/vkbuffer.hpp"


// Create regular grid in the space of the point cloud. Returns vector with 3d points of grid
std::vector<glm::vec3> create_regular_grid(float const& grid_resolution, std::vector<glm::vec3> const& point_cloud, std::vector<uint32_t>& );

// Find scalar value for each point in the grid. Returns vector with these values
std::vector<int> calculate_distance_field(std::vector<glm::vec3> const& grid_vertices, std::vector<glm::vec3> const& point_cloud_vertices);

// Classifies vertices as positive, negative.
// If the isovalue picked coincides with a scalar value of the distance field, the user will be asked to input another value to ensure manifoldness.
// Generally, the isovalue will be an integer + 0.5 to ensure this condition
std::vector<glm::vec3> classify_grid_vertices();

#endif //MARCHING_CUBES_POINT_CLOUD_DISTANCE_FIELD_HPP
