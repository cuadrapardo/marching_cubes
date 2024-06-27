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

std::vector<glm::vec3> query_case_table(std::vector<unsigned int> const& grid_val);

#endif //MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP
