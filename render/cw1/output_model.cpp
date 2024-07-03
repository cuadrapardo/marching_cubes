//
// Created by Carolina Cuadra Pardo on 7/1/24.
//

#include "output_model.hpp"

#include <fstream>
#include <iostream>

/* Given a std::vector of 3d positions where each triplet defines a triangle, output an obj file. */
void write_OBJ(std::vector<glm::vec3> const& positions, std::string const& filename) {
    assert(positions.size()%3 == 0); //Must be a multiple of 3

    std::string out_filename = "reconstructed_surface";
    out_filename += ".obj";
    std::ofstream objFile(out_filename);
    if (!objFile.is_open()) {
        std::cerr << "Failed to open file: " << out_filename << "\n";
        return;
    }
    // Write vertices
    for (const auto& pos : positions) {
        objFile << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
    }
    // Write faces
    for (size_t i = 0; i < positions.size(); i += 3) {
        objFile << "f " << i + 1 << " " << i + 2 << " " << i + 3 << "\n";
    }

    objFile.close();
    if (objFile.fail()) {
        std::cerr << "Failed to write to file: " << filename << "\n";
    } else {
        std::cout << "Successfully wrote to file: " << filename << "\n";
    }
}
