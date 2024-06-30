//
// Created by Carolina Cuadra Pardo on 6/30/24.
//
#include <glm/vec3.hpp>

#ifndef MARCHING_CUBES_POINT_CLOUD_TEST_SCENE_HPP
#define MARCHING_CUBES_POINT_CLOUD_TEST_SCENE_HPP

#include "../render/cw1/point_cloud.hpp"

PointCloud create_test_scene();

void recalculate_test_scene(std::vector<unsigned int> const& vertex_configuration, PointCloud& test_scene);

std::vector<unsigned int> get_test_scene_edges();

std::pair<std::vector<unsigned int>, std::vector<glm::vec3>> classify_cube_edges(std::vector<unsigned int> const& edge_indices,
std::vector<unsigned int> const& vertex_values);

#endif //MARCHING_CUBES_POINT_CLOUD_TEST_SCENE_HPP
