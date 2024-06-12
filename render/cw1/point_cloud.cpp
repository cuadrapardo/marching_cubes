//
// Created by Carolina Cuadra Pardo on 6/12/24.
//

#include "point_cloud.hpp"
#include <unordered_set>

PointCloud obj_to_pointcloud(SimpleModel& obj_file) {
    PointCloud pCloud;
    for (const auto& point : obj_file.dataUntextured.positions) {
        pCloud.points.insert(point);
    }

    for (const auto& point : obj_file.dataTextured.positions) {
        pCloud.points.insert(point);
    }

    return pCloud;
}
