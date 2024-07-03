//
// Created by Carolina Cuadra Pardo on 7/1/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_OUTPUT_MODEL_HPP
#define MARCHING_CUBES_POINT_CLOUD_OUTPUT_MODEL_HPP

#include <vector>
#include <string>

#include <glm/vec3.hpp>

void write_OBJ(std::vector<glm::vec3> const& vertices, std::string const& filename);

#endif //MARCHING_CUBES_POINT_CLOUD_OUTPUT_MODEL_HPP
