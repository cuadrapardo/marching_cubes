//
// Created by Carolina Cuadra Pardo on 6/12/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP
#define MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP

#include <unordered_set>
#include "simple_model.hpp"


struct PointCloud {
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> colors;

};

PointCloud obj_to_pointcloud(SimpleModel& obj_file); //TODO: do I even need this function?

#endif //MARCHING_CUBES_POINT_CLOUD_POINT_CLOUD_HPP
