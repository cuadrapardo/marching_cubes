//
// Created by Carolina Cuadra Pardo on 6/24/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP
#define MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP

#include <vector>

struct Cube {
    unsigned int vertex_indices[8];
};

void query_case_table(std::vector<unsigned int> const& grid_val);

#endif //MARCHING_CUBES_POINT_CLOUD_SURFACE_RECONSTRUCTION_HPP
