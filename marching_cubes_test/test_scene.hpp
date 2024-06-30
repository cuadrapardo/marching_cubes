//
// Created by Carolina Cuadra Pardo on 6/30/24.
//
#include <glm/vec3.hpp>

#ifndef MARCHING_CUBES_POINT_CLOUD_TEST_SCENE_HPP
#define MARCHING_CUBES_POINT_CLOUD_TEST_SCENE_HPP

#include "../render/cw1/point_cloud.hpp"

PointCloud create_test_scene();

void recalculate_test_scene(unsigned int const (&vertex_configuration)[8], PointCloud& test_scene);

#endif //MARCHING_CUBES_POINT_CLOUD_TEST_SCENE_HPP
