//
// Created by Carolina Cuadra Pardo on 7/3/24.
//

#include "halfedge.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>



int get_next_halfedge(unsigned int const& halfedge_idx) {
    //Get index of face
    unsigned int face_idx = halfedge_idx / 3;
    unsigned int idx_within_face = halfedge_idx % 3;

    switch (idx_within_face) {
        case 0:
            return (3 * face_idx + 1);
            break;
        case 1:
            return  (3 * face_idx + 2);
            break;
        case 2:
            return(3 * face_idx + 0);
            break;
    }
    return -1; // Invalid
}

int get_previous_halfedge(unsigned int const& halfedge_idx) {
    //Get index of face
    unsigned int face_idx = halfedge_idx / 3;
    unsigned int idx_within_face = halfedge_idx % 3;

    switch (idx_within_face) {
        case 0:
            return (3 * face_idx + 2);
            break;
        case 1:
            return  (3 * face_idx);
            break;
        case 2:
            return(3 * face_idx + 1);
            break;
    }
    return -1; // Invalid
}

// Given a face index, returns the indices to its 3 halfedges.
std::array<int, 3> get_halfedges(unsigned int const& face_idx) {
    std::array<int, 3> face_halfedges;

    face_halfedges[0] = 3 * face_idx;
    face_halfedges[1] = 3 * face_idx + 1;
    face_halfedges[2] = 3 * face_idx + 2;

    return face_halfedges;
}



/* For each halfedge, find its other half */
void HalfEdgeMesh::set_other_halves() {
    unsigned int face_idx = 0;
    for(unsigned int vertex_idx = 0; vertex_idx < faces.size(); vertex_idx += 3) {
        //Find indices of its 3 halfedges
        std::array<int, 3> halfedges = get_halfedges(face_idx);

        //For each halfedge, find its other half i.e - the half edge in the opposite direction
        //TODO: unfinished


        face_idx++;
    }


}


/* Reads in an obj and returns a HalfEdgeMesh */
HalfEdgeMesh obj_to_halfedge(char const* file_path) {
    HalfEdgeMesh mesh;
    std::ifstream obj_file(file_path);
    if (!obj_file.is_open()) {
        std::cerr << "Failed to open the file: " << file_path << std::endl;
        return HalfEdgeMesh{};
    }

    std::string line;

    while (std::getline(obj_file, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "v") { // Vertices
            glm::vec3 position;
            iss >> position.x >> position.y >> position.z;
            mesh.vertex_positions.push_back(glm::vec3{
                position.x,
                position.y,
                position.z
            });

            mesh.vertex_outgoing_halfedge.push_back(-1);
        } else if (token == "vn") { // Normals
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            // Assuming normals are listed in the same order as vertices
            mesh.vertex_normals.push_back(
                    glm::vec3 {
                        normal.x,
                        normal.y,
                        normal.z
                    }
                    );
        } else if (token == "f") { // Faces
            std::vector<int> face_indices;
            std::string vertex;
            while (iss >> vertex) {
                std::istringstream vertex_idx_stream(vertex);
                std::string index;
                std::getline(vertex_idx_stream, index, '/'); // Read vertex index
                mesh.faces.push_back(std::stoi(index) - 1); // OBJ indices are 1-based, convert to 0-based.
            }

            // For each face, create 3 halfedges.
            unsigned int idx_2 = face_indices.size() - 1;
            unsigned int idx_1 = face_indices.size() - 2;
            unsigned int idx_0 = face_indices.size() - 3;

            // HalfEdge 0 - From vertex 0 to vertex 1
            mesh.halfedges_opposite.push_back(-1);
            mesh.halfedges_vertex_to.push_back(idx_1);
            // Set vertex outgoing halfedge
            mesh.vertex_outgoing_halfedge[idx_0] = mesh.halfedges_opposite.size() - 1;

            // HalfEdge 1 - From vertex 1 to vertex 2
            mesh.halfedges_opposite.push_back(-1);
            mesh.halfedges_vertex_to.push_back(idx_2);
            // Set vertex outgoing halfedge
            mesh.vertex_outgoing_halfedge[idx_1] = mesh.halfedges_opposite.size() - 1;

            // HalfEdge 2 - From vertex 2 to vertex 0
            mesh.halfedges_opposite.push_back(-1);
            mesh.halfedges_vertex_to.push_back(idx_0);
            // Set vertex outgoing halfedge
            mesh.vertex_outgoing_halfedge[idx_2] = mesh.halfedges_opposite.size() - 1;
        }
    }
    obj_file.close();

    mesh.set_other_halves();
}
