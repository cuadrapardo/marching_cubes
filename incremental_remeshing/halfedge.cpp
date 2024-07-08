//
// Created by Carolina Cuadra Pardo on 7/3/24.
//

#include "halfedge.hpp"

#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <algorithm>



int HalfEdgeMesh::get_next_halfedge(unsigned int const& halfedge_idx) {
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

int HalfEdgeMesh::get_previous_halfedge(unsigned int const& halfedge_idx) {
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
std::array<int, 3> HalfEdgeMesh::get_halfedges(unsigned int const& face_idx) {
    std::array<int, 3> face_halfedges;

    face_halfedges[0] = 3 * face_idx;
    face_halfedges[1] = 3 * face_idx + 1;
    face_halfedges[2] = 3 * face_idx + 2;

    return face_halfedges;
}


/* Iterates through one ring and returns vertex indices of those belonging to the given vertex's one ring */
std::unordered_set<unsigned int> HalfEdgeMesh::get_one_ring_vertices(const unsigned int& vertex_idx) {
    std::unordered_set<unsigned int> one_ring;
    //Get first directed edge
    int const& fde = vertex_outgoing_halfedge[vertex_idx];
    one_ring.insert(halfedges_vertex_to[fde]);
    int current_he_idx = get_previous_halfedge(fde);
    while(fde != current_he_idx) {
        unsigned int other_half = halfedges_opposite[current_he_idx];
        if(other_half == fde) {
            break;
        }
        one_ring.insert(halfedges_vertex_to[other_half]);
        current_he_idx = get_previous_halfedge(other_half);
    }
    return one_ring;
}



/* For each halfedge, find its other half */
void HalfEdgeMesh::set_other_halves() {
    unsigned int face_idx = 0;
    for(unsigned int vertex_idx = 0; vertex_idx < faces.size(); vertex_idx += 3) {
        //Find indices of its 3 halfedges
        std::array<int, 3> halfedges = get_halfedges(face_idx);

        //For each halfedge, find its other half i.e - the half edge in the opposite direction
        /* The goal here is to find an edge that has the following: Given a halfedge edge E whose other half OE we are trying to find:
         * -----> E's previous halfedge in the triangle will point TOWARDS E's FROM vertex.
         *        Therefore:  ----> OE must point TOWARDS E's previous halfedge
         * -----> The vertex E is pointing TOWARDS must be the same vertex OE's previous edge is pointing towards
         *  We can successfully find a halfedge's other half by finding the edge that fulfills these requirements */

        for(auto& halfedge : halfedges ) {
            int previous_he = get_previous_halfedge(halfedge);
            int vertex_from = halfedges_vertex_to[previous_he];


            //Find all edges that point towards previous vertex
            auto it = halfedges_vertex_to.begin();

            while (it != halfedges_vertex_to.end()) {
                it = std::find(it, halfedges_vertex_to.end(), vertex_from );
                unsigned int candidate_he = (std::distance(halfedges_vertex_to.begin(), it));
                //Check if this candidate halfedge is actually the other half
                int candidate_he_previous_he = get_previous_halfedge(candidate_he);
                int candidate_he_vertex_from = halfedges_vertex_to[candidate_he_previous_he];

                int vertex_to_halfedge = halfedges_vertex_to[halfedge];
                int vertex_to_candidate = halfedges_vertex_to[candidate_he];
                if(candidate_he_vertex_from == vertex_to_halfedge && vertex_from == vertex_to_candidate) {
                // Found other half
                     halfedges_opposite[halfedge] = candidate_he;
                    break;
                    }
                   ++it; // Move past the current found item
                }
        }
        face_idx++;
    }
}


/* Reads in an obj and returns a HalfEdgeMesh */
HalfEdgeMesh obj_to_halfedge(char const* file_path) {
    //TODO: check for boundary to generalise
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
            std::string vertex;
            while (iss >> vertex) {
                std::istringstream vertex_idx_stream(vertex);
                std::string index;
                std::getline(vertex_idx_stream, index, '/'); // Read vertex index
                mesh.faces.push_back(std::stoi(index) - 1); // OBJ indices are 1-based, convert to 0-based.
            }

            // For each face, create 3 halfedges.
            unsigned int idx_2 = mesh.faces[ mesh.faces.size() - 1];
            unsigned int idx_1 = mesh.faces[mesh.faces.size() - 2];
            unsigned int idx_0 = mesh.faces[ mesh.faces.size() - 3];

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

/* Finds average edge length, useful as a target lentgh for tangential relaxation */
float HalfEdgeMesh::get_mean_edge_length() {
    float total_length;
    for(unsigned int he_idx; he_idx < halfedges_vertex_to.size(); he_idx++){
        unsigned int previous_he_idx = get_previous_halfedge(he_idx);

        unsigned int vertex_from = halfedges_vertex_to[previous_he_idx];
        float edge_length = glm::distance(
                vertex_positions[vertex_from],
                vertex_positions[halfedges_vertex_to[he_idx]]
                );
        total_length += edge_length;
    }
    return (total_length / halfedges_vertex_to.size());
}

/* Checks whether a surface is manifold by checking the following conditions:
 *  Triangle mesh is 2-manifold iff:
        * 1 -----------> all edges share two faces
        * If there is a halfedge which does not have another half,  there is an edge with only ONE adjacent face
        * 2 ----------> no pinch points at vertices (single cycle around each vertex). NOT TESTING AGAINST SELF INTERSECTIONS */
bool HalfEdgeMesh::check_manifold(){
    //Check if any of the opposite's are set to -1 (i.e-- they do not have another half)
    auto it = std::find(halfedges_opposite.begin(), halfedges_opposite.end(), -1);
    if (it != halfedges_opposite.end()) {
        return false; //Not manifold- condition 1 not true
    }

    //Check for pinch point
    for(unsigned int vertex = 0; vertex < vertex_positions.size(); vertex++) {
        //Check number of half edges that have this vertex as endpoint.
        int degree = std::count(halfedges_vertex_to.begin(), halfedges_vertex_to.end(), vertex);

        std::unordered_set<unsigned int> one_ring = get_one_ring_vertices(vertex);

        if(degree != one_ring.size()) {
         //Pinch point
         return 0;
        }
    }
    return 1;
}


/* Splits edge at its midpoint. Creates 2 new faces, 1 new vertex, and modifies 2 existing faces & their halfedges*/
void HalfEdgeMesh::edge_split(const unsigned int& he_idx) {
    // The starting halfedges (he_idx & he_opposite) are the ones on the desired edge to be split.

    // Face 0 - the face which contains he_idx
    //Halfedge 0
    unsigned int const he_vertex_to = halfedges_vertex_to[he_idx];

    //Halfedge 1
    unsigned int const& he_idx_1 = get_next_halfedge(he_idx);
    unsigned int const& he_idx_1_vertex = halfedges_vertex_to[he_idx_1];
    unsigned int const he_idx_1_opp = halfedges_opposite[he_idx_1]; // make a copy because this will be modified


    //Halfedge 2
    unsigned int const& he_idx_2 = get_next_halfedge(he_idx_1);
    unsigned int const& he_idx_2_vertex = halfedges_vertex_to[he_idx_2];

    // Face 1 - the face which contains he_idx's opposite.
    //Halfedge 0
    unsigned int const& he_opposite = halfedges_opposite[he_idx];
    unsigned int he_opp_vertex_to = halfedges_vertex_to[he_opposite];

    //Halfedge 1
    unsigned int const& he_opp_idx_1 =  get_next_halfedge(he_opposite);
    unsigned int const& he_opp_1_vertex = halfedges_vertex_to[he_opp_idx_1];

    //Halfedge 2
    unsigned int const& he_opp_idx_2 =  get_next_halfedge(he_opp_idx_1);
    unsigned int const& he_opp_2_vertex = halfedges_vertex_to[he_opp_idx_2];
    unsigned int const he_opp_idx_2_opp = halfedges_opposite[he_opp_idx_2];  // make a copy because this will be modified


    // DEBUG checks.
    assert(he_vertex_to == he_opp_2_vertex);
    assert(he_opp_vertex_to == he_idx_2_vertex);

    //Insert a vertex at the midpoint of the edge that will be split
    glm::vec3 pos_1 = vertex_positions[he_vertex_to];

    glm::vec3 pos_2 = vertex_positions[he_opp_vertex_to];

    glm::vec3 midpoint = ( pos_1 + pos2 ) / 2.0f; //position of vertex that splits the edge at midpoint

    // Add vertex to data structure
    vertex_positions.push_back(midpoint);
    unsigned int new_vertex_idx = vertex_positions.size() - 1;
    vertex_outgoing_halfedge.push_back(-1);

    // Add new face - Face 3
    faces.push_back(new_vertex_idx);
    faces.push_back(he_vertex_to);
    faces.push_back(he_idx_1_vertex);

    unsigned int start_he_idx = halfedges_vertex_to.size() - 1;

    //Halfedge 0
    halfedges_opposite.push_back(start_he_idx + 4); // First he of Face 4
    halfedges_vertex_to.push_back(he_vertex_to);
    vertex_outgoing_halfedge[new_vertex_idx] = halfedges_opposite.size() - 1;

    //Halfedge 1
    halfedges_opposite.push_back(he_idx_1_opp); // Previous he of edge 1, face 0
    halfedges_vertex_to.push_back(he_idx_1_vertex);
    vertex_outgoing_halfedge[he_vertex_to] = halfedges_opposite.size() - 1;
    halfedges_opposite[he_idx_1_opp] = halfedges_opposite.size() - 1; // Append previously existing HE with correct other half.

    //Halfedge 2
    halfedges_opposite.push_back(he_idx_1);
    halfedges_vertex_to.push_back(new_vertex_idx);
    vertex_outgoing_halfedge[he_idx_1_vertex] = halfedges_opposite.size() - 1;
    halfedges_opposite[he_idx_1] = halfedges_opposite.size() - 1;

    // Add new face - Face 2
    faces.push_back(he_opp_2_vertex);
    faces.push_back(new_vertex_idx);
    faces.push_back(he_opp_1_vertex);

    //Halfedge 0
    halfedges_opposite.push_back(start_he_idx + 1);
    halfedges_vertex_to.push_back(new_vertex_idx);
    vertex_outgoing_halfedge[he_opp_2_vertex] = halfedges_opposite.size() - 1;

    //Halfedge 1
    halfedges_opposite.push_back(he_opp_idx_2);
    halfedges_vertex_to.push_back(he_opp_1_vertex);
    vertex_outgoing_halfedge[new_vertex_idx] = halfedges_opposite.size() - 1;
    halfedges_opposite[he_opp_idx_2] = halfedges_opposite.size() - 1;

    //Halfedge 2
    halfedges_opposite.push_back(he_opp_idx_2_opp);
    halfedges_vertex_to.push_back(he_opp_2_vertex);
    vertex_outgoing_halfedge[he_opp_1_vertex] = halfedges_opposite.size() - 1;
    halfedges_opposite[he_opp_idx_2_opp] = halfedges_opposite.size() - 1;

    //Update original faces adjacent to edge with new vertex

    //Face adjacent to he_idx
    unsigned int face_0 = he_idx / 3; // Adjacent face to he_idx
    unsigned int face_1 = he_opposite / 3; // Opposite face to he_idx
    for(unsigned int vertex_idx = 0; vertex_idx < 3; vertex_idx++) {
        if(faces[(face_0*3)+ vertex_idx] == he_vertex_to) {
            faces[(face_0*3)+ vertex_idx] = new_vertex_idx;
        }
        if(faces[(face_1*3)+ vertex_idx] == he_vertex_to) {
            faces[(face_1*3)+ vertex_idx] = new_vertex_idx;
        }
    }
    //Update edges of old faces with new vertex to.
    //Face 0
    halfedges_vertex_to[he_opp_idx_2] = new_vertex_idx;
    //Face 1
    halfedges_vertex_to[he_idx] = new_vertex_idx;


}

/* Halfedge collapse:
 *
An edge collapse transformation K -> K' that collapses the edge {i, j} ∈ K
is a legal move if and only if the following conditions are satisfied (proof in [6]):

1. For all vertices {k} adjacent to both {i} and {j} ({i, k} ∈ K and {j, k} ∈ K),
   {i, j, k} is a face of K.

2. If {i} and {j} are both boundary vertices, {i, j} is a boundary edge.

3. K has more than 4 vertices if neither {i} nor {j} are boundary vertices,
   or K has more than 3 vertices if either {i} or {j} are boundary vertices.

   Taken from:
   Hoppe, H., Derose, T., Duchamp, T., Mcdonald, J. and Stuetzle, Mesh Optimization.
   Available from: https://www.hhoppe.com/meshopt.pdf.

    //TODO: check if boundary. Right now it is okay to not check as the input is assumed to be from Marching Cubes application

 * */
void HalfEdgeMesh::edge_collapse(const unsigned int& he_idx) {
    unsigned int const& vertex_to = halfedges_vertex_to[he_idx];
    unsigned int const& vertex_from = halfedges_vertex_to[halfedges_opposite[he_idx]];

    //Iterate through one ring
    std::unordered_set<unsigned int> p_onering = get_one_ring_vertices(vertex_to);
    std::unordered_set<unsigned int> q_onering = get_one_ring_vertices(vertex_from);



    //Check for legality of operation
    std::unordered_set<unsigned int> intersection;
    for (const int& vertex : p_onering) {
        if (q_onering.find(vertex) != q_onering.end()) {
            intersection.insert(vertex);
        }
    }

    //In order for edge collapse to be legal, vertices which belong to the intersection must form a triangle with v0 and v1 and the target edge
    std::unordered_set<unsigned int> connected_vertices; // Vertices which form triangles with the target edge
    connected_vertices.insert(vertex_to);

    // Triangle belonging to he_idx
    unsigned int const& he_1_idx = get_next_halfedge(he_idx);
    connected_vertices.insert(halfedges_vertex_to[he_1_idx]);

    unsigned int const& he_2_idx = get_next_halfedge(he_1_idx);
    connected_vertices.insert(halfedges_vertex_to[he_2_idx]);

    //Triangle belonging to he_opposite_idx
    unsigned int const& he_opposite_idx = halfedges_opposite[he_idx];
    connected_vertices.insert(halfedges_vertex_to[he_opposite_idx]);

    unsigned int const& he_opp_idx_1 = get_next_halfedge(he_opposite_idx);
    connected_vertices.insert(halfedges_vertex_to[he_opp_idx_1]);

    unsigned int const& he_opp_idx_2 = get_next_halfedge(he_opp_idx_1);
    connected_vertices.insert(halfedges_vertex_to[he_opp_idx_2]);

    if(connected_vertices.size() != intersection.size()) {
        //Illegal operation.
        return;
    }

    // Check if all elements are equal (they must be), if not it must mean there are more triangles connecting to p and q which
    // do not form a triangle with edge pq
    for (const int& vertex : intersection) {
        if (connected_vertices.find(vertex) == connected_vertices.end()) {
            return;
        }
    }

    //Update data structure to reflect edge collapse









}
