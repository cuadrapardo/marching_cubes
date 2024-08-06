//
// Created by Carolina Cuadra Pardo on 7/1/24.
//

#include "output_model.hpp"
#include "../labutils/render_constants.hpp"
#include "mesh.hpp"

#include <fstream>
#include <iostream>

/* Given a std::vector of 3d positions where each triplet defines a triangle, output an obj file. */
void write_OBJ(IndexedMesh const& indexedMesh, std::string const& out_filename) {
    assert(indexedMesh.face_indices.size()%3 == 0); //Must be a multiple of 3

    std::ofstream objFile(out_filename);
    if (!objFile.is_open()) {
        std::cerr << "Failed to open file: " << out_filename << "\n";
        return;
    }

    // Write unique vertices
    for (unsigned int vertex  = 0 ;vertex < indexedMesh.positions.size(); vertex++) {
        glm::vec3 pos = indexedMesh.positions[vertex];
        objFile << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
    }

    // Write faces
    for (unsigned int i = 0; i < indexedMesh.face_indices.size(); i += 3) {
        objFile << "f ";
        for (unsigned int j = 0; j < 3; ++j) {
            int vertex_idx = indexedMesh.face_indices[i + j] + 1; // obj indices are 1 based
            objFile << vertex_idx << " ";
        }
        objFile << "\n";
    }

    objFile.close();
    if (objFile.fail()) {
        std::cerr << "Failed to write to file: " << out_filename << "\n";
    } else {
        std::cout << "Successfully wrote to file: " << out_filename << "\n";
    }
}
