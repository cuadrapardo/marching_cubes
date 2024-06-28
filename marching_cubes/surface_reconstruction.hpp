//
// Created by Carolina Cuadra Pardo on 6/24/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP
#define MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP

#include <vector>
#include <glm/vec3.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include "mc_tables.h"

unsigned int get_case(unsigned int const (&vertex_values)[8], float const& isolevel);

glm::vec3 linear_interpolation(glm::vec3 const& point_1, glm::vec3 const& point_2,
                               unsigned int const& scalar_1, unsigned int const& scalar_2, float const& isovalue);

std::vector<glm::vec3> query_case_table(std::vector<unsigned int> const& grid_val);

#endif //MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP
