//
// Created by Carolina Cuadra Pardo on 7/3/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_HALFEDGE_HPP
#define MARCHING_CUBES_POINT_CLOUD_HALFEDGE_HPP

#include <vector>
#include <array>
#include <unordered_set>
#include <glm/vec3.hpp>


struct HalfEdgeMesh {
    // Vertex information
    std::vector<glm::vec3> vertex_positions;
    std::vector<glm::vec3> vertex_normals;
    std::vector<int> vertex_outgoing_halfedge; // aka. first directed edge of a vertex

    // Halfedge information
    std::vector<int> halfedges_opposite; //aka. other halves
    std::vector<int> halfedges_vertex_to;

    std::vector<unsigned int> faces; // Every 3 entries is a face.

    void set_other_halves();

    bool check_manifold();


    // Helper functions
    std::array<int, 3> get_halfedges(unsigned int const& face_idx);
    int get_previous_halfedge(unsigned int const& halfedge);
    int get_next_halfedge(unsigned int const& halfedge);
    std::unordered_set<unsigned int> get_one_ring_vertices(unsigned int const& vertex_idx);

    //Mesh operations
    void edge_collapse(unsigned int const& edge_idx);
    void edge_split(unsigned int const& edge_idx);
    void edge_flip(unsigned int const& edge_idx);

    //Incremental remeshing operations
    float get_mean_edge_length();
    void split_long_edges(const float& high_edge_length);
    void collapse_short_edges(const float& high_edge_length, const float& low_edge_length);
    void equalize_valences();
    void tangential_relaxation();
    void project_to_surface();

    void remesh(float const& input_target_edge_length);
};

HalfEdgeMesh obj_to_halfedge(char const* path);



#endif //MARCHING_CUBES_POINT_CLOUD_HALFEDGE_HPP
