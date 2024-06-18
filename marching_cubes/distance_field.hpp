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
    //Buffer for rendering
    labutils::Buffer grid_positions_buffer;
    labutils::Buffer grid_color_buffer;
    labutils::Buffer grid_scalar_value_buffer;

    //Values
    std::vector<glm::vec3> grid_positions;
    std::vector<float> grid_scalar_value;
    std::uint32_t vertex_count; //is this neccesary???
};

// Create regular grid in the space of the point cloud
std::vector<glm::vec3> create_regular_grid(int const& grid_resolution, std::vector<glm::vec3> const& point_cloud);

std::vector<float> calculate_distance_field(std::vector<glm::vec3> const& grid_vertices); //TODO: implement

//Creates buffer for variables needed for DistanceField struct
DistanceField create_distance_field(std::vector<glm::vec3> const& grid_positions, std::vector<float> grid_scalar_value,
                                    labutils::VulkanContext const& window, labutils::Allocator const& allocator );




// Find scalar value for each point in the grid
void calculate_distance_field();

#endif //MARCHING_CUBES_POINT_CLOUD_DISTANCE_FIELD_HPP
