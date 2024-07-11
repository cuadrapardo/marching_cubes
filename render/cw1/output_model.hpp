//
// Created by Carolina Cuadra Pardo on 7/1/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_OUTPUT_MODEL_HPP
#define MARCHING_CUBES_POINT_CLOUD_OUTPUT_MODEL_HPP

#include <vector>
#include <string>

#include <glm/vec3.hpp>
#include "mesh.hpp"

void write_OBJ(IndexedMesh const& indexedMesh, std::string const& filename);

#endif //MARCHING_CUBES_POINT_CLOUD_OUTPUT_MODEL_HPP
