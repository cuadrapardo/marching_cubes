//
// Created by Carolina Cuadra Pardo on 6/12/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP
#define MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP

#include <unordered_set>
#include "simple_model.hpp"

//TODO: add vertex attribute for size (dependant on scalar value)
struct PointCloud {
    labutils::Buffer positions_buffer;
    labutils::Buffer colors;

    std::vector<glm::vec3> positions;
    std::uint32_t vertex_count;
};


PointCloud create_pointcloud(std::vector<glm::vec3> positions, labutils::VulkanContext const& window, labutils::Allocator const& allocator );

#endif //MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP
