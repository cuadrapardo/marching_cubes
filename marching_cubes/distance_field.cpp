//
// Created by Carolina Cuadra Pardo on 6/17/24.
//

#include "distance_field.hpp"
#include "../third_party/glm/include/glm/glm.hpp"
#include "../render/labutils/vkutil.hpp" //TODO: add include folder to make includes nicer
#include "../render/labutils/to_string.hpp"
#include "../render/labutils/error.hpp"
#include <cstring>


std::vector<glm::vec3> create_regular_grid(int const& grid_resolution, std::vector<glm::vec3> const& point_cloud) {
    std::vector<glm::vec3> grid_positions;
    //Determine size of point cloud (bounding box)
    glm::vec3 min = {std::numeric_limits<float>::max(),std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    glm::vec3 max = {std::numeric_limits<float>::lowest(),std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

    for (const auto& point : point_cloud) {
        min.x = glm::min(min.x, point.x);
        min.y = glm::min(min.y, point.y);
        min.z = glm::min(min.z, point.z);

        max.x = glm::max(max.x, point.x);
        max.y = glm::max(max.y, point.y);
        max.z = glm::max(max.z, point.z);
    }

    glm::vec3 extents = glm::abs(max-min);

    float scale = 1.0f / grid_resolution;

    glm::ivec3 grid_boxes = {
            extents.x / scale,
            extents.y / scale,
            extents.z / scale,
    };

    for (unsigned int i = 0; i <= grid_boxes.x + 1 ; i++) {
        for(unsigned int j = 0; j <= grid_boxes.y + 1; j++) {
            for(unsigned int k = 0; k <= grid_boxes.z + 1; k++) {
                grid_positions.emplace_back(
                            min.x + (i * scale),
                            min.y + (j * scale),
                            min.z + (k * scale)
                        );

            }
        }
    }

    return grid_positions;
}


//For each grid vertex, find distance to nearest point cloud vertices
std::vector<float> calculate_distance_field(std::vector<glm::vec3> const& grid_vertices, std::vector<glm::vec3> const& point_cloud_vertices) {
    std::vector<float> grid_scalar_value;

    for(auto const& grid_vertex : grid_vertices) {
        float distance = std::numeric_limits<float>::max();
        for(auto const& pcloud_vertex: point_cloud_vertices) {
            float d = glm::distance(grid_vertex, pcloud_vertex);
            distance = (d < distance) ? d : distance;
        }
        grid_scalar_value.push_back(distance);
    }
    return grid_scalar_value;
}