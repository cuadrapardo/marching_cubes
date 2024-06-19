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

struct DistanceField {
    std::vector<glm::vec3> grid_positions;
    std::vector<float> grid_scalar_value;
};

// Create regular grid in the space of the point cloud
std::vector<glm::vec3> create_regular_grid(int const& grid_resolution, std::vector<glm::vec3> const& point_cloud);

std::vector<float> calculate_distance_field(std::vector<glm::vec3> const& grid_vertices, std::vector<glm::vec3> const& point_cloud);




// Find scalar value for each point in the grid
void calculate_distance_field();

#endif //MARCHING_CUBES_POINT_CLOUD_DISTANCE_FIELD_HPP
