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
#include <algorithm>

/* Find min/max extents of the model in 3D space. Returns BoundingBox */
BoundingBox get_bounding_box(std::vector<glm::vec3> const& point_cloud) {
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
    return BoundingBox{
        .min = min,
        .max = max
    };
}


/* Creates regular grid with given resolution (1/resolution). The grid is described in terms of 3D space units, not grid units.
 * Returns a vector of grid positions. Populates edge vector as the indices of its two ends. */
std::vector<glm::vec3> create_regular_grid(float const& grid_resolution, std::vector<uint32_t>& grid_edges, BoundingBox& model_bbox) {
    std::cout << "Creating regular grid" << std::endl;
    std::vector<glm::vec3> grid_positions;

    float scale = 1.0f / grid_resolution;

    glm::vec3 extents = glm::abs(model_bbox.max - model_bbox.min);

    glm::ivec3 grid_boxes = {
            extents.x / scale,
            extents.y / scale,
            extents.z / scale,
    };

    auto get_index = [grid_boxes](unsigned int i, unsigned int j, unsigned int k) -> unsigned int {
        return i * (grid_boxes.y + 2) * (grid_boxes.z + 2) + j * (grid_boxes.z + 2) + k;
    }; // note this will only work in a loop structured like the one below

    for (unsigned int i = 0; i <= grid_boxes.x + 1 ; i++) {
        for(unsigned int j = 0; j <= grid_boxes.y + 1; j++) {
            for(unsigned int k = 0; k <= grid_boxes.z + 1; k++) {
                // Add point
                grid_positions.emplace_back(
                        model_bbox.min.x + (i * scale),
                        model_bbox.min.y + (j * scale),
                        model_bbox.min.z + (k * scale)
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


/* For each grid vertex, find distance to nearest point cloud vertices (integer value).
 * Returns vector of scalar values at grid vertices */
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

/* Classifies grid vertices as negative and positive. Negative - 0 ; Positive - 1
 * In order to ensure manifoldness, 0.5 is added to the isovalue (which avoids the case of isovalue == grid scalar value),
 * since scalar values are ints. Returns a vector of positive/negative classification */
std::vector<unsigned int> classify_grid_vertices(std::vector<int> const& grid_scalar_values, int const& isovalue) {
    std::vector<unsigned int> grid_vertex_classification;
    float const isovalue_05 = isovalue + 0.5;
    for(auto const& grid_value : grid_scalar_values) {
//        assert(grid_value != isovalue_05); //This should never happen, but if it does, it will not produce a manifold surface.
        if(grid_value >= isovalue_05) {
            grid_vertex_classification.push_back(1); // Positive
        }
        if(grid_value < isovalue_05) {
            grid_vertex_classification.push_back(0); // Negative
        }
    }
    return grid_vertex_classification;
}

/* Classifies grid edges as:
 * -------------------------> 0 : negative - two endpoints of the edge are negative,
 * -------------------------> 1 : positive - two endpoints of the edge are positive,
 * -------------------------> 2 : bipolar - one endpoint is positive and the other negative.
 * Returns a vector of positive/negative/bipolar classification AND its color, represented in RGB */
std::pair<std::vector<unsigned int>, std::vector<glm::vec3>> classify_grid_edges(std::vector<unsigned int> const& grid_scalar_values, BoundingBox const& model_bbox,
                                                                                 float const& grid_resolution) {
    glm::vec3 extents = glm::abs(model_bbox.max - model_bbox.min);
    float scale = 1.0f / grid_resolution;

    glm::ivec3 grid_boxes = {
            extents.x / scale,
            extents.y / scale,
            extents.z / scale,
    };

    std::cout << "Classifying grid edges" << std::endl;
    std::vector<unsigned int> edge_values;
    std::vector<glm::vec3> edge_colors;
    edge_colors.resize(grid_scalar_values.size()*2);

    auto get_index = [grid_boxes](unsigned int i, unsigned int j, unsigned int k) -> unsigned int {
        return i * (grid_boxes.y + 2) * (grid_boxes.z + 2) + j * (grid_boxes.z + 2) + k;
    }; // note this will only work in a loop structured like the one below

    auto get_edge_value = [](unsigned int v1, unsigned int v2) -> unsigned int {
        if (v1 == 1 && v2 == 1) {
            return 1; // Positive edge
        } else if (v1 == 0 && v2 == 0) {
            return 0; // Negative edge
        } else {
            return 2; // Bipolar edge
        }
    };

    auto get_edge_color = [](unsigned int value) -> glm::vec3 {
        switch (value) {
            case 0: //Negative
                return glm::vec3{0.0f, 0.f, 0.f};
                break;
            case 1: //Positive
                return glm::vec3{1.0f, 1.0f, 1.0f};
                break;
            case 2: //Bipolar
                return glm::vec3{1.0f, 0.f, 1.0f};
                break;
        }
    };

    for (unsigned int i = 0; i <= grid_boxes.x + 1; i++) {
        for (unsigned int j = 0; j <= grid_boxes.y + 1; j++) {
            for (unsigned int k = 0; k <= grid_boxes.z + 1; k++) {

                unsigned int v1, v2, edge_value;
                std::size_t idx1, idx2;
                glm::vec3 edge_color;

                // Check edge vertices' values
                if (i < grid_boxes.x + 1) {
                    idx1 = (get_index(i, j, k));
                    idx2 = get_index(i + 1, j, k);
                    v1 = grid_scalar_values[idx1];
                    v2 = grid_scalar_values[idx2];

                    edge_value = get_edge_value(v1, v2);
                    edge_color = get_edge_color(edge_value);

                    edge_colors[idx1] = edge_color;
                    edge_colors[idx2] = edge_color;
                    edge_values.push_back(edge_value);
                }
                if (j < grid_boxes.y + 1) {
                    idx1 = (get_index(i, j, k));
                    idx2 = get_index(i , j + 1, k);
                    v1 = grid_scalar_values[idx1];
                    v2 = grid_scalar_values[idx2];

                    edge_value = get_edge_value(v1, v2);
                    edge_color = get_edge_color(edge_value);

                    edge_colors[idx1] = edge_color;
                    edge_colors[idx2] = edge_color;
                    edge_values.push_back(edge_value);
                }
                if (k < grid_boxes.z + 1) {
                    idx1 = (get_index(i, j, k));
                    idx2 = get_index(i , j , k + 1);
                    v1 = grid_scalar_values[idx1];
                    v2 = grid_scalar_values[idx2];

                    edge_value = get_edge_value(v1, v2);
                    edge_color = get_edge_color(edge_value);

                    edge_colors[idx1] = edge_color;
                    edge_colors[idx2] = edge_color;
                    edge_values.push_back(edge_value);
                }
            }
        }
    }
    return std::make_pair(edge_values, edge_colors);
}

/* In order to ensure that the surface created actually encompasses the model, the bounding box needs to be
 * big enough. This increments each axis by 500% of the input bounding box's size on each axis*/
void BoundingBox::add_padding(const float& increment_factor) {
//    float x_extent = max.x - min.x;
//    float y_extent = max.y - min.y;
//    float z_extent = max.z - min.z;

    glm::vec3 extent = max - min;

    glm::vec3 increment = (increment_factor * extent);

    max = max + increment;
    min = min - increment;

}

/* Returns a vector of the point values of the distance field, normalised and linearly inversely transformed */
std::vector<int> apply_point_size_transfer_function(std::vector<int> const& distance_field) {
    std::vector<int> distance_field_transfer;
    int min_val = *std::min_element(distance_field.begin(), distance_field.end());
    int max_val = *std::max_element(distance_field.begin(), distance_field.end());


    //Normalise & apply transfer function
    for(auto point : distance_field) {
        float normalised = (float)(point - min_val) / (float)(max_val - min_val);
        float inverse_normalised = 1.0f - normalised;

        int p_size = (int)(inverse_normalised * 4) + 1; //Scale from 1 to 5
        distance_field_transfer.push_back(p_size);
    }

    return distance_field_transfer;
}

//float calculate_isovalue(PointCloud const& distance_field) {
//
//}
