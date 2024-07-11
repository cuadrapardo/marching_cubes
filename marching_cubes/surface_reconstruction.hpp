//
// Created by Carolina Cuadra Pardo on 6/24/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP
#define MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP

#include <vector>
#include <glm/vec3.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include "distance_field.hpp"
#include "../render/cw1/mesh.hpp"

unsigned int get_case(unsigned int const (&vertex_values)[8]);

glm::vec3 linear_interpolation(glm::vec3 const& point_1, glm::vec3 const& point_2,
                               unsigned int const& scalar_1, unsigned int const& scalar_2, float const& isovalue);

IndexedMesh query_case_table(std::vector<unsigned int> const& grid_values, std::vector<glm::vec3> const& grid_positions,
                                        std::vector<int> const& grid_scalar_values, float const& grid_resolution, BoundingBox const& model_bbox, float const& input_isovalue);

std::vector<glm::vec3> query_case_table_test(std::vector<unsigned int> const& grid_values, std::vector<glm::vec3> const& grid_positions,
                                             float const& input_isovalue);


#endif //MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP
