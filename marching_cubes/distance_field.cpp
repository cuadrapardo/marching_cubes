//
// Created by Carolina Cuadra Pardo on 6/17/24.
//

#include "distance_field.hpp"
#include "../third_party/glm/include/glm/glm.hpp"
#include "../render/labutils/vkutil.hpp" //TODO: add include folder to make includes nicer
#include "../render/labutils/to_string.hpp"
#include "../render/labutils/error.hpp"
#include <cstring>
#include <iostream>


std::vector<glm::vec3> create_regular_grid(float const& grid_resolution, std::vector<glm::vec3> const& point_cloud, std::vector<uint32_t>& grid_edges) {
    std::cout << "Creating regular grid" << std::endl;
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

    auto get_index = [grid_boxes](unsigned int i, unsigned int j, unsigned int k) -> unsigned int {
        return i * (grid_boxes.y + 2) * (grid_boxes.z + 2) + j * (grid_boxes.z + 2) + k;
    }; // note this will only work in a loop structured like the one below

    for (unsigned int i = 0; i <= grid_boxes.x + 1 ; i++) {
        for(unsigned int j = 0; j <= grid_boxes.y + 1; j++) { // j 26 i 4 k 0
            for(unsigned int k = 0; k <= grid_boxes.z + 1; k++) {
                grid_positions.emplace_back(
                            min.x + (i * scale),
                            min.y + (j * scale),
                            min.z + (k * scale)
                        );

                // Add edges
                if (i < grid_boxes.x + 1) {
                    grid_edges.emplace_back(get_index(i, j, k));
                    grid_edges.emplace_back(get_index(i + 1, j, k));
                }
                if (j < grid_boxes.y + 1) {
                    grid_edges.emplace_back(get_index(i, j, k));
                    grid_edges.emplace_back(get_index(i, j + 1, k));
                }
                if (k < grid_boxes.z + 1) {
                    grid_edges.emplace_back(get_index(i, j, k));
                    grid_edges.emplace_back(get_index(i, j, k + 1));
                }



            }
        }
    }

    return grid_positions;
}


//For each grid vertex, find distance to nearest point cloud vertices
std::vector<int> calculate_distance_field(std::vector<glm::vec3> const& grid_vertices, std::vector<glm::vec3> const& point_cloud_vertices) {
    std::cout << "Calculating distance field with " << grid_vertices.size() << " vertices" << std::endl;
    std::vector<int> grid_scalar_value;

    for(auto const& grid_vertex : grid_vertices) {
        float distance = std::numeric_limits<float>::max();
        for(auto const& pcloud_vertex: point_cloud_vertices) {
            int d = glm::distance(grid_vertex, pcloud_vertex);
            distance = (d < distance) ? d : distance;
        }
        grid_scalar_value.push_back(distance);
    }
    return grid_scalar_value;
}


std::vector<bool> classify_grid_vertices(std::vector<int> const& grid_scalar_values, int const& isovalue) {
    std::vector<bool> grid_vertex_classification;
    float const isovalue_05 = isovalue + 0.5;
    for(auto const& grid_value : grid_scalar_values) {
        assert(grid_value != isovalue_05); //This should never happen, but if it does, it will not produce a manifold surface.
        if(grid_value > isovalue_05) {
            grid_vertex_classification.push_back(true);
        }
        if(grid_value < isovalue_05) {
            grid_vertex_classification.push_back(false);
        }
    }
    return grid_vertex_classification;

}
