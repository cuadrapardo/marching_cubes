//
// Created by Carolina Cuadra Pardo on 6/12/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP
#define MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP

#include <unordered_set>
#include "simple_model.hpp"

template <>
struct std::hash<glm::vec3> {
    std::size_t operator()(const glm::vec3& v) const {
        std::size_t h1 = std::hash<float>()(v.x);
        std::size_t h2 = std::hash<float>()(v.y);
        std::size_t h3 = std::hash<float>()(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// Equality operator for glm::vec3
bool operator==(const glm::vec3& lhs, const glm::vec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

struct PointCloud {
    std::unordered_set<glm::vec3> points;
};

PointCloud obj_to_pointcloud(SimpleModel& obj_file); //TODO: do I even need this function?

#endif //MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP
