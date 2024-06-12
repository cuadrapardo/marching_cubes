//
// Created by Carolina Cuadra Pardo on 6/12/24.
//

#include "point_cloud.hpp"
#include <unordered_set>
#include <algorithm>

PointCloud obj_to_pointcloud(SimpleModel& obj_file) {
    PointCloud pCloud;
    //Add points
    pCloud.points.insert(pCloud.points.end(), obj_file.dataTextured.positions.begin(),  obj_file.dataTextured.positions.end());
    pCloud.points.insert(pCloud.points.end(), obj_file.dataUntextured.positions.begin(),  obj_file.dataUntextured.positions.end());

    //Add color (default to red)
    pCloud.colors.resize(pCloud.points.size());
    std::fill(pCloud.colors.begin(), pCloud.colors.end(), glm::vec3{1.0f, 0.0f, 0.0f});

    return pCloud;
}
