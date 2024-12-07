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

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;

    void add_padding(const float& increment_factor);
};


//Gets min/max coordinates representing point cloud bounding box.
BoundingBox get_bounding_box(std::vector<glm::vec3> const& point_cloud);

// Create regular grid in the space of the point cloud. Returns vector with 3d points of grid
std::vector<glm::vec3> create_regular_grid(float const& grid_resolution, std::vector<uint32_t>& , BoundingBox&);

// Find scalar value for each point in the grid. Returns vector with these values
std::vector<int> calculate_distance_field(std::vector<glm::vec3> const& grid_vertices, std::vector<glm::vec3> const& point_cloud_vertices);

// Classifies vertices as positive, negative. Returns vector with false = negative, true = positive
// If the isovalue picked coincides with a scalar value of the distance field, the user will be asked to input another value to ensure manifoldness.
// Generally, the isovalue will be an integer + 0.5 to ensure this condition
std::vector<unsigned int> classify_grid_vertices(std::vector<int> const& grid_scalar_values, int const& isovalue);

//
std::pair<std::vector<unsigned int>, std::vector<glm::vec3>> classify_grid_edges(std::vector<unsigned int> const& grid_vertex_classification,  BoundingBox const& model_bbox,
                                                                                 float const& grid_resolution);

std::vector<int> apply_point_size_transfer_function(std::vector<int> const&);

//float calculate_isovalue(PointCloud const& distance_field);

#endif //MARCHING_CUBES_POINT_CLOUD_DISTANCE_FIELD_HPP
