//
// Created by Carolina Cuadra Pardo on 6/12/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP
#define MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP

#include <unordered_set>
#include "simple_model.hpp"

struct PointCloud {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<float> point_size;

    void set_color(glm::vec3 const& color);
    void set_size(unsigned int const& size);
};

struct PointBuffer {
    labutils::Buffer positions;
    labutils::Buffer color;
    labutils::Buffer scale;
    std::uint32_t vertex_count;
};

struct LineBuffer {
    labutils::Buffer indices;
    labutils::Buffer color;
    std::uint32_t vertex_count;
};


PointBuffer create_pointcloud_buffers(std::vector<glm::vec3> positions, std::vector<glm::vec3> color, std::vector<float> scale,
                             labutils::VulkanContext const& window, labutils::Allocator const& allocator);

LineBuffer create_index_buffer(std::vector<uint32_t> const&, std::vector<glm::vec3> const&, labutils::VulkanContext const& window, labutils::Allocator const& allocator);

#endif //MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP
