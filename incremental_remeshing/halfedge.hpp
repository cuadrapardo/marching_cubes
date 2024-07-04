//
// Created by Carolina Cuadra Pardo on 7/3/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_HALFEDGE_HPP
#define MARCHING_CUBES_POINT_CLOUD_HALFEDGE_HPP

#include <vector>
#include <array>
#include <glm/vec3.hpp>


struct HalfEdgeMesh {
    // Vertex information
    std::vector<glm::vec3> vertex_positions;
    std::vector<glm::vec3> vertex_normals;
    std::vector<int> vertex_outgoing_halfedge;

    // Halfedge information
//    std::vector<int> halfedges;
    std::vector<int> halfedges_opposite;
    std::vector<int> halfedges_vertex_to;

    std::vector<unsigned int> faces; // Every 3 entries is a face.

    void set_other_halves();


    // Helper functions
    std::array<int, 3> get_halfedges(unsigned int const& face_idx);
    int get_previous_halfedge(unsigned int const& halfedge);
    int get_next_halfedge(unsigned int const& halfedge);
};

HalfEdgeMesh obj_to_halfedge(char const* path);



#endif //MARCHING_CUBES_POINT_CLOUD_HALFEDGE_HPP
