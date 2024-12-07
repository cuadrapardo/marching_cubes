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
#include <cmath>

void HalfEdgeMesh::reset() {
    vertex_positions.clear();
    vertex_normals.clear();
    vertex_outgoing_halfedge.clear();
    faces.clear();
    halfedges_opposite.clear();
    halfedges_vertex_to.clear();
}

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

float HalfEdgeMesh::get_edge_length(const unsigned int& halfedge) {
    glm::vec3 const& vertex_to_position = vertex_positions[halfedges_vertex_to[halfedge]];
    unsigned int vertex_from_idx = get_vertex_from(halfedge);
    glm::vec3 const& vertex_from_position = vertex_positions[vertex_from_idx];
    return glm::distance(vertex_to_position,vertex_from_position);

}


int HalfEdgeMesh::get_vertex_from(const unsigned int& halfedge) {
    unsigned int const& previous_he = get_previous_halfedge(halfedge);
    return halfedges_vertex_to[previous_he];
}

int HalfEdgeMesh::get_face(const unsigned int& halfedge) {
    return (halfedge/3);
}

std::array<unsigned int, 3> HalfEdgeMesh::get_face_vertices(unsigned int const& face ) {
    std::array<unsigned int, 3> vertices;

    vertices[0] = faces[face*3 + 0];
    vertices[1] = faces[face*3 + 1];
    vertices[2] = faces[face*3 + 2];

    return vertices;
};

/* Following implementation seen in : https://iquilezles.org/articles/normals/ */
void HalfEdgeMesh::calculate_normals() {
    vertex_normals.resize(vertex_positions.size());
    for( int i=0; i < vertex_positions.size(); i++ ) vertex_normals[i] = glm::vec3(0.0f);

    for( int i=0; i< faces.size() ; i+=3 )
    {
        const int ia = faces[i + 0];
        const int ib = faces[i + 1];
        const int ic = faces[i + 2];

        const glm::vec3 e1 = vertex_positions[ia] - vertex_positions[ib];
        const glm::vec3 e2 = vertex_positions[ic] - vertex_positions[ib];
        const glm::vec3 no = glm::cross( e1, e2 );

        vertex_normals[ia] += no;
        vertex_normals[ib] += no;
        vertex_normals[ic] += no;
    }

    for( int i=0; i< vertex_positions.size(); i++ ) {
        vertex_normals[i] = glm::normalize( vertex_normals[i] );
    }
}



/* Given a he_idx which will be collapsed, deletes face it belongs to, deleting its 3 halfedges.
 * Reconnects the pair of other halves belonging to the triangle's edges (excluding the collapsed edge) */
void HalfEdgeMesh::delete_face(const unsigned int& he_idx) {
    //Triangle belonging to the edge collapsed
    unsigned int face_idx_0 = get_face(he_idx);

    unsigned int next_he = get_next_halfedge(he_idx);
    unsigned int prev_he = get_previous_halfedge(he_idx);
    //Save other halves to reconnect later on with neighbouring triangles
    unsigned int he_next_oh = halfedges_opposite[next_he];
    unsigned int he_prev_oh = halfedges_opposite[prev_he];

    unsigned int const& non_edge_vertex = halfedges_vertex_to[get_next_halfedge(he_idx)]; //This vertices' fde will need to be updated.
                                                                                          // It is the vertex that belongs to an adjacent triangle to the collapsed edge,
                                                                                          // but isn't at either endpoint of the collapsed edge.
    //Update FDE of non_edge_vertex
    vertex_outgoing_halfedge[non_edge_vertex] = halfedges_opposite[get_next_halfedge(he_idx)];

    //Get he of index 0 within this face (first halfedge of the face)
    auto halfedges = get_halfedges(face_idx_0);

    //Reset connectivity of other halves belonging to edges in the to-be-deleted triangle
    halfedges_opposite[he_next_oh] = he_prev_oh;
    halfedges_opposite[he_prev_oh] = he_next_oh;

    //Reset first directed edges from collapsed triangle
    vertex_outgoing_halfedge[non_edge_vertex] = he_next_oh;

    //Flag these for deletion
    for(int const& he : halfedges) {
        halfedges_opposite[he] = -1;
        halfedges_vertex_to[he] = -1;
    }
}


/* Iterates through one ring and returns vertex indices of those belonging to the given vertex's one ring */
std::unordered_set<unsigned int> HalfEdgeMesh::get_one_ring_vertices(const unsigned int& vertex_idx) {
    std::unordered_set<unsigned int> one_ring;
    //Get first directed edge
    int const& fde = vertex_outgoing_halfedge[vertex_idx];
    one_ring.insert(halfedges_vertex_to[fde]);
    int current_he_idx = get_previous_halfedge(fde);
    assert(halfedges_vertex_to[current_he_idx] == vertex_idx);
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
    for (unsigned int vertex_idx = 0; vertex_idx < faces.size(); vertex_idx += 3) {
        //Find indices of its 3 halfedges
        std::array<int, 3> halfedges = get_halfedges(face_idx);

        //For each halfedge, find its other half i.e - the half edge in the opposite direction
        /* The goal here is to find an edge that has the following: Given a halfedge edge E whose other half OE we are trying to find:
         * -----> E's previous halfedge in the triangle will point TOWARDS E's FROM vertex.
         *        Therefore:  ----> OE must point TOWARDS E's previous halfedge
         * -----> The vertex E is pointing TOWARDS must be the same vertex OE's previous edge is pointing towards
         *  We can successfully find a halfedge's other half by finding the edge that fulfills these requirements */

        for (auto& halfedge: halfedges) {
            int vertex_from = get_vertex_from(halfedge);

            //Find all edges that point towards previous vertex (vertex from)
            for (unsigned int otherhalf = 0; otherhalf < halfedges_vertex_to.size(); otherhalf++) {
                if (halfedges_vertex_to[otherhalf] != vertex_from) {
                    continue; //Not found candidate other half
                }

                //Check if this candidate halfedge is actually the other half
                int vertex_to = halfedges_vertex_to[halfedge];
                int vertex_to_candidate = halfedges_vertex_to[otherhalf];

                int vertex_from_candidate = get_vertex_from(otherhalf);
                if (vertex_from == vertex_to_candidate && vertex_to == vertex_from_candidate) {
                    // Found other half
                    halfedges_opposite[halfedge] = otherhalf;
                    halfedges_opposite[otherhalf] = halfedge;
                    break;
                } else {
                    continue;
                }
            }
        }
        face_idx++;
    }
}


/* Reads in an obj and returns a HalfEdgeMesh */
HalfEdgeMesh obj_to_halfedge(char const* file_path) {
    //TODO: check for boundary to generalise

    std::cout << "Creating halfedge data structure from reconstructed surface" << std::endl;
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

        if (line.empty() || line[0] == '#') {
            continue;
        }


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

    std::cout << "Pairing other halves in mesh structure. "
                 "Total n of halfedges: " << mesh.halfedges_opposite.size() << std::endl;
    mesh.set_other_halves();

    return mesh;
}

/* Finds average edge length, useful as a target lentgh for tangential relaxation */
float HalfEdgeMesh::get_mean_edge_length() {
    float total_length = 0;
    for(unsigned int he_idx = 0; he_idx < halfedges_vertex_to.size(); he_idx++){
        float edge_length = get_edge_length(he_idx);
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
    for(unsigned int i = 0; i < halfedges_opposite.size(); i++ ) {
        if(halfedges_opposite[i] == -1) {
            return false;
        }
    }

    //Check for pinch point
    for(unsigned int vertex = 0; vertex < vertex_positions.size(); vertex++) {
        //Check number of half edges that have this vertex as endpoint.
        unsigned int degree = 0;
        for(unsigned int j = 0; j < halfedges_vertex_to.size(); j++ ) {
            if(halfedges_vertex_to[j] == vertex) {
                degree++;
                continue;
            }
        }
        //Check whether this number matches the vertex one ring
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

    glm::vec3 midpoint = ( pos_1 + pos_2 ) / 2.0f; //position of vertex that splits the edge at midpoint

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
 *  An edge collapse transformation K -> K' that collapses the edge {i, j} ∈ K is a legal move
 *  if and only if the following conditions are satisfied:
 *
 * 1. For all vertices {k} adjacent to both {i} and {j} ({i, k} ∈ K and {j, k} ∈ K),
 * {i, j, k} is a face of K.
 *
 * 2. If {i} and {j} are both boundary vertices, {i, j} is a boundary edge.
 *
 * 3. K has more than 4 vertices if neither {i} nor {j} are boundary vertices,
 * or K has more than 3 vertices if either {i} or {j} are boundary vertices.
 *
 * In total this operation removes two triangles , one vertex & three edges (6 halfedges).
 * Removing triangles changes indexing in the face arrays & consequently the halfedge arrays. A way to
 *  Taken from:
 * Hoppe, H., Derose, T., Duchamp, T., Mcdonald, J. and Stuetzle, Mesh Optimization.
 * Available from: https://www.hhoppe.com/meshopt.pdf. */
//TODO: check if boundary. Right now it is okay to not check as the input is assumed to be from Marching Cubes application (and therefore, manifold)
bool HalfEdgeMesh::edge_collapse(const unsigned int& he_idx, const float& high_edge_length) {
    unsigned int const& vertex_to = halfedges_vertex_to[he_idx];
    unsigned int const vertex_from = get_vertex_from(he_idx);

    //Iterate through one ring
    std::unordered_set<unsigned int> p_onering = get_one_ring_vertices(vertex_from);
    std::unordered_set<unsigned int> q_onering = get_one_ring_vertices(vertex_to);



    //Check for legality of operation
    std::unordered_set<unsigned int> intersection;
    for (const int& vertex : p_onering) {
        if (q_onering.find(vertex) != q_onering.end()) {
            intersection.insert(vertex);
        }
    }
    //Add p & q to the intersection
    intersection.insert(vertex_to);
    intersection.insert(vertex_from);

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
        return false;
    }

    // Check if all elements are equal (they must be), if not it must mean there are more triangles connecting to p and q which
    // do not form a triangle with edge pq
    for (const int& vertex : intersection) {
        if (connected_vertices.find(vertex) == connected_vertices.end()) {
            return false;
        }
    }

    //Even though edge collapse is legal, check whether collapsing this edge would produce a longer edge (and undo work done in edge split)
    //In order to check this, get the one ring of the vertex that will be deleted. Check what the distance is to the vertex where they will be connected.
    //If this is larger than the high threshold, do not carry out edge collapse.
    for(unsigned int he = 0; he < halfedges_vertex_to.size(); he++)  {
        if(halfedges_vertex_to[he] == vertex_from) {
            //Check how long the edge would be with the new vertex.
            unsigned int const& he_vertex_from = get_vertex_from(halfedges_vertex_to[he]);
            glm::vec3 const& vertex_to_position = vertex_positions[vertex_to]; // Halfedge the collapsed edge points to. (i.e the vertex that will remain after the collapse)
            glm::vec3 const& vertex_from_position = vertex_positions[he_vertex_from];
            if(glm::distance(vertex_to_position,vertex_from_position) >= high_edge_length) {
                return false; //Collapsing he_idx will create longer edges.
            }
        }
    }
    //Update data structure to reflect edge collapse
    vertex_outgoing_halfedge[vertex_to] = halfedges_opposite[get_previous_halfedge(halfedges_opposite[he_idx])]; //To avoid clashes, set the fde to an edge which will NOT get collapsed.

    //Flag the halfedges of the two triangles adjacent to the edge for deletion.
    //Triangle 0 - triangle belonging to the other half of the edge collapsed
    unsigned int deleted_face_0 = get_face(he_idx);
    auto deleted_halfedges_0 = get_halfedges(deleted_face_0);
    delete_face(he_idx);

    //Triangle 1 - triangle belonging to the other half of the edge collapsed
    unsigned int deleted_face_1 = get_face(he_opposite_idx);
    auto deleted_halfedges_1 = get_halfedges(deleted_face_1);
    delete_face(he_opposite_idx);

    //Update halfedges pointing to the deleted vertex.
    for(unsigned int halfedge = 0; halfedge < halfedges_vertex_to.size(); halfedge++) {
        int& curr_vertex_to = halfedges_vertex_to[halfedge];
        if(curr_vertex_to == -1) { continue; } // This halfedge is flagged for deletion, continue
        if(curr_vertex_to == vertex_from) {
            curr_vertex_to = vertex_to;
        }
    }

    //Update halfedge indices
    for(unsigned int halfedge = 0; halfedge < halfedges_opposite.size(); halfedge++) {
        auto curr_he = halfedges_opposite[halfedge];
        if(curr_he == -1) { continue; }
        if(curr_he > deleted_halfedges_0[2]) { //If the idx of the opposite is above the deleted he index for face 0, shift by -3
            halfedges_opposite[halfedge] -=3;
        }
        if(curr_he > deleted_halfedges_1[2]) { //If the idx of the opposite is above the deleted he index for face 1, shift by -3
            halfedges_opposite[halfedge] -=3;
        }
    }
    for(unsigned int vertex = 0; vertex < vertex_outgoing_halfedge.size(); vertex++) {
        auto outgoing_he = vertex_outgoing_halfedge[vertex];
        assert(outgoing_he != -1); // All FDE should have been redirected beforehand if they were going to be deleted.
        if(outgoing_he > deleted_halfedges_0[2]) { //If the idx of the opposite is above the deleted he index for face 0, shift by -3
            vertex_outgoing_halfedge[vertex] -=3;
        }
        if(outgoing_he > deleted_halfedges_1[2]) { //If the idx of the opposite is above the deleted he index for face 1, shift by -3
            vertex_outgoing_halfedge[vertex] -=3;
        }
    }



    //Update faces - 2 will be deleted

    faces[deleted_face_0*3 + 0] = -1;
    faces[deleted_face_0*3 + 1] = -1;
    faces[deleted_face_0*3 + 2] = -1;

    faces[deleted_face_1*3 + 0] = -1;
    faces[deleted_face_1*3 + 1] = -1;
    faces[deleted_face_1*3 + 2] = -1;

    //Delete flagged faces
    for (auto it = faces.begin(); it != faces.end();) {
        // Check if the current face is flagged for deletion
        if (*it == -1 && *(it + 1) == -1 && *(it + 2) == -1) {
            // Erase the face (3 elements)
            it = faces.erase(it, it + 3);
        } else {
            // Move to the next face
            it += 3;
        }
    }

    //Update vertex indices in faces
    for(auto& vertex : faces ) {
        if(vertex == vertex_from) {
            vertex = vertex_to;
        }
        if(vertex > vertex_from) {
            vertex--;
        }
    }

    //Update vertex indices in vertex_to
    for(auto& vertex : halfedges_vertex_to) {
        if(vertex == -1) { continue;}
        if(vertex == vertex_from) {
            vertex = vertex_to;
        }
        if(vertex > vertex_from) {
            vertex--;
        }
    }

    //Remove vertex
    vertex_positions.erase(vertex_positions.begin() + vertex_from);
    vertex_outgoing_halfedge.erase(vertex_outgoing_halfedge.begin() + vertex_from);

    //Remove halfedges
    auto it_opposite = halfedges_opposite.begin();
    auto it_vertex_to = halfedges_vertex_to.begin();

    while (it_opposite != halfedges_opposite.end() && it_vertex_to != halfedges_vertex_to.end()) {
        if (*it_opposite == -1 || *it_vertex_to == -1) {
            it_opposite = halfedges_opposite.erase(it_opposite);
            it_vertex_to = halfedges_vertex_to.erase(it_vertex_to);
        } else {
            ++it_opposite;
            ++it_vertex_to;
        }
    }

    return true;

}


/* Flips one edge (two halfedges)
 * - Changes the vertex to for 6 halfedges in the triangles adjacent to the edge
 * - Changes the vertices of 2 faces alongside the edge.
 * TODO: In theory this can all be done in one function, which is called twice. */
void HalfEdgeMesh::edge_flip(const unsigned int& he_idx) {
    //Change outgoing he for vertices to avoid them being modified with the edge flip operation
    auto recalculate_fde = [this](unsigned int const& he_idx, unsigned int const& he_next_idx,  unsigned int const& he_prev_idx ) {
        const unsigned int& vertex_0 = this->halfedges_vertex_to[he_idx];
        const unsigned int& vertex_1 = this->halfedges_vertex_to[he_next_idx];
        const unsigned int& vertex_2 = this->halfedges_vertex_to[he_prev_idx];

        vertex_outgoing_halfedge[vertex_2] = halfedges_opposite[he_prev_idx];
        vertex_outgoing_halfedge[vertex_0] = halfedges_opposite[get_previous_halfedge(halfedges_opposite[he_idx])];
        vertex_outgoing_halfedge[vertex_1] = halfedges_opposite[he_next_idx];
    };


    //Triangle belonging to he_idx
    const unsigned int he_next_idx = get_next_halfedge(he_idx);
    const unsigned int he_prev_idx = get_previous_halfedge(he_idx);
    const unsigned int he_prev_oh =  halfedges_opposite[he_prev_idx];
    const unsigned int he_prev_to = halfedges_vertex_to[he_prev_idx];
    const unsigned int face_idx_0 = get_face(he_idx);

    //Save copies for later usage, as these values will be modified
    const unsigned int he_next_oh = halfedges_opposite[he_next_idx];
    const unsigned int he_next_to = halfedges_vertex_to[he_next_idx];

    //Triangle belonging to he_idx's other half
    const unsigned int oh_idx = halfedges_opposite[he_idx];
    const unsigned int oh_next_idx = get_next_halfedge(oh_idx);
    const unsigned int oh_prev_idx = get_previous_halfedge(oh_idx);
    const unsigned int oh_prev_oh =  halfedges_opposite[oh_prev_idx];
    const unsigned int oh_prev_to = halfedges_vertex_to[oh_prev_idx];
    const unsigned int oh_next_oh = halfedges_opposite[oh_next_idx];
    const unsigned int oh_next_to = halfedges_vertex_to[oh_next_idx];

    const unsigned int oh_vertex_to = halfedges_vertex_to[oh_idx];
    const unsigned int face_idx_1 = get_face(oh_idx);

    assert(he_prev_to == halfedges_vertex_to[oh_idx]);
    assert(he_next_to == halfedges_vertex_to[he_prev_oh]);
//    assert(he_next_to == oh_prev_to);

    recalculate_fde(he_idx, he_next_idx, he_prev_idx);
    recalculate_fde(oh_idx, oh_next_idx, oh_prev_idx);

    //Change he_idx's vertex_to
    halfedges_vertex_to[he_idx] = halfedges_vertex_to[he_next_idx];
    //Change oh_idx's vertex to
    halfedges_vertex_to[oh_idx] = halfedges_vertex_to[oh_next_idx];

    //"Switch" values halfedges
    halfedges_opposite[he_next_idx] = he_prev_oh;
    halfedges_opposite[he_prev_oh] = he_next_idx;
    halfedges_vertex_to[he_next_idx] = he_prev_to;

    halfedges_opposite[he_prev_idx] = oh_next_oh;
    halfedges_opposite[oh_next_oh] = he_prev_idx;
    halfedges_vertex_to[he_prev_idx] = oh_next_to;

    //Same thing with the other triangle
    halfedges_opposite[oh_next_idx] = oh_prev_oh;
    halfedges_opposite[oh_prev_oh] = oh_next_idx;
    halfedges_vertex_to[oh_next_idx] = oh_prev_to;

    halfedges_opposite[oh_prev_idx] = he_next_oh; //These values have been modified, so use the stored values
    halfedges_opposite[he_next_oh] = oh_prev_idx;
    halfedges_vertex_to[oh_prev_idx] = he_next_to;

    assert(halfedges_vertex_to[he_idx] == he_next_to);
    assert(halfedges_vertex_to[he_next_idx] == oh_vertex_to);
    assert(halfedges_vertex_to[he_prev_idx] == oh_next_to);

    //Change the face vertices for the two faces adjacent to the edge
    faces[face_idx_0*3 + 0] = halfedges_vertex_to[he_idx];
    faces[face_idx_0*3 + 1] = halfedges_vertex_to[he_next_idx];
    faces[face_idx_0*3 + 2] = halfedges_vertex_to[he_prev_idx];

    faces[face_idx_1*3 + 0] = halfedges_vertex_to[oh_idx];
    faces[face_idx_1*3 + 1] = halfedges_vertex_to[oh_next_idx];
    faces[face_idx_1*3 + 2] = halfedges_vertex_to[oh_prev_idx];

}

/* Splits all edges longer than high_edge_length at their midpoint */
void HalfEdgeMesh::split_long_edges(const float& high_edge_length) {
    for(unsigned int edge = 0; edge < halfedges_vertex_to.size(); edge++) {
        float edge_length = get_edge_length(edge);
        if(edge_length < high_edge_length) {
            continue;
        }
        //Edge is too long- split at midpoint
        edge_split(edge);
    }
}

/* Performs halfedge collapse on edges shorter than the threshold low_edge_length.
 * The algorithm might create edges which are long and undo the work during the edge split so this function
 * checks whether that would happen before performing the split. */
void HalfEdgeMesh::collapse_short_edges(const float& high_edge_length, const float& low_edge_length) {
    unsigned int edge = 0;
    while (edge < halfedges_vertex_to.size()) {
        if (get_edge_length(edge) >= low_edge_length) {
            edge++;
            continue;
        }
        bool collapsed = edge_collapse(edge, high_edge_length);
        if(!collapsed) {edge++;}

    }
}

/* Equalizes vertex valences by flipping edges.
 * Checks whether the deviation to the target valence decreases, if not, edge is flipped back */
void HalfEdgeMesh::equalize_valences() {
    auto get_deviation = [this](std::unordered_set<unsigned int>& vertices) -> int {
        int deviation = 0;
        for(auto const& vertex : vertices) {
            deviation += abs(get_one_ring_vertices(vertex).size() - INTERIOR_TARGET_VALENCE);
        }
        return deviation;
    };

    for(unsigned int edge = 0; edge < halfedges_vertex_to.size(); edge++ ) {
        unsigned int face_0 = get_face(edge);
        unsigned int face_1 = get_face(halfedges_opposite[edge]);

        //Get vertices of two triangles adjacent to halfedges
        auto vertices_0 = get_face_vertices(face_0);
        auto vertices_1 = get_face_vertices(face_1);

        std::unordered_set<unsigned int> unique_vertices;
        for(auto const& vertex : vertices_0) {
            unique_vertices.insert(vertex);

        }
        for(auto const& vertex : vertices_1) {
            unique_vertices.insert(vertex);
        }

        int pre_flip_deviation = get_deviation(unique_vertices);

        edge_flip(edge);

        int post_flip_deviation = get_deviation(unique_vertices);

        if(pre_flip_deviation <= post_flip_deviation) {
            edge_flip(edge); // Undo operation if valence was not improved
        }
    }

}

/* Iterative smoothing filter for the mesh. Vertex movement is constrained to the vertex tangent plane */
void HalfEdgeMesh::tangential_relaxation() {
    for(unsigned int vertex_idx = 0; vertex_idx < vertex_positions.size(); vertex_idx++ ) {
        glm::vec3 const& position = vertex_positions[vertex_idx];
        glm::vec3 const& normal = vertex_normals[vertex_idx];

        auto one_ring = get_one_ring_vertices(vertex_idx);
        glm::vec3 barycentre = glm::vec3{0.0f, 0.0f, 0.0f};

        for(const auto vertex : one_ring) {
            barycentre = barycentre + vertex_positions.at(vertex);
        }
        barycentre = barycentre / (float)one_ring.size();
        glm::vec3 updated_position = barycentre + glm::dot(normal, (position - barycentre)) * normal;

        vertex_positions[vertex_idx] = updated_position;
    }
}



/* Performs remeshing according to procedures described in
 * Botsch, M. and Kobbelt, L. 2004. A remeshing approach to multiresolution modeling.
 * This consists of the following operations:
 * - Find mean edge length. This will be used as the target length (although this is not necessary, the user could input their own value)
 * - Split long edges
 * - Collapse short edges
 * - Tangential relaxation (smooth mesh without deformation)
 * Another description of the algorithm can be seen at: Botsch, M. 2010. Polygon mesh processing. Natick, Mass.: A K Peters. pages 100 - 102*/
void HalfEdgeMesh::remesh(float const& input_target_edge_length, unsigned int const& n_iterations) {
    float target_edge_length;
    if(input_target_edge_length == 0) {
        target_edge_length = get_mean_edge_length();
    } else {
        target_edge_length = input_target_edge_length;
    }
    float low = (4.0f/5.0f) * target_edge_length; // the thresholds 4/5 and 4/3
    float high = (4.0f/3.0f) * target_edge_length; // are essential to converge to a uniform edge length

    for(unsigned int remeshing_iterations = 0; remeshing_iterations < n_iterations; remeshing_iterations++) {
        split_long_edges(high);

        collapse_short_edges(high, low);

        equalize_valences();

        calculate_normals();

        tangential_relaxation();
    }
}

/* Mesh triangle quality metrics:
 * Calculate mean triangle area using Heron's formula
 * Calculate triangle area range
 * Calculate mean triangle aspect ratio (Aspect ratio of a triangle is the ratio of the longest edge to shortest edge (so equilateral triangle has aspect ratio 1).  */
void HalfEdgeMesh::calculate_triangle_area_metrics() {
    float min_area = std::numeric_limits<float>::max();
    float max_area = std::numeric_limits<float>::min();
    float total_area = 0;
    float total_aspect_ratio = 0;
    float total_area_squared = 0;
    for(unsigned int tri = 0; tri < faces.size()/3; tri++) {
        //Calculate area
        auto vertices = get_face_vertices(tri);
        float edge_0_length = glm::distance(vertex_positions[vertices[0]], vertex_positions[vertices[1]]);
        float edge_1_length = glm::distance(vertex_positions[vertices[1]], vertex_positions[vertices[2]]);
        float edge_2_length = glm::distance(vertex_positions[vertices[2]], vertex_positions[vertices[0]]); // ??? is glm::distance here fine?

        float half_perimeter = (edge_0_length + edge_1_length + edge_2_length) / 2.0f;

        float area = sqrt(
                half_perimeter * (half_perimeter - edge_0_length) * (half_perimeter - edge_1_length) * (half_perimeter - edge_2_length)
                );
        //Calculate aspect ratio
        float min_edge = std::min(edge_0_length, edge_1_length);
        min_edge = std::min(min_edge, edge_2_length);
        float max_edge = std::max(edge_0_length, edge_1_length);
        max_edge = std::max(max_edge, edge_2_length);

        float aspect_ratio = max_edge/min_edge;



        if (area < min_area) {
            min_area = area;
        }
        if (area > max_area) {
            max_area = area;
        }
        total_area += area;
        total_aspect_ratio += aspect_ratio;
        total_area_squared += area * area;
    }
    average_triangle_area = total_area / ((float)faces.size()/3);
    mean_triangle_aspect_ratio = total_aspect_ratio / ((float)faces.size()/3);
    triangle_area_range = max_area - min_area;

    float variance = (total_area_squared/((float)faces.size()/3)) - (average_triangle_area * average_triangle_area);
    triangle_area_standard_deviation = sqrt(variance);
}

/* Calculates the Hausforff Distance between two HalfEdge Meshes.
 * if we have two sets A and B then H(A,B) is found by computing
 * the minimum distance d(a,B) for each point a ∈ A and then taking the maximum
*  of those values [Botsch, Mario/Kobbelt, Leif/Pauly, Mark. Polygon Mesh Processing]
 *  In this case, we calculate the Hausdorff Distance wrt the the original point set */
float HalfEdgeMesh::calculate_hausdorff_distance(std::vector<glm::vec3> const& original_points) {
    //For each point in the original pointset, calculate distance to closest point in halfedge mesh
    std::vector<float> minimum_distance(original_points.size(), std::numeric_limits<float>::max());
    for(unsigned int p = 0; p < original_points.size(); p++) {
        glm::vec3 const& point = original_points[p];
        for(auto const& mesh_point : vertex_positions) {
            float distance = glm::distance(point, mesh_point);
            if(distance < minimum_distance[p]) {
                minimum_distance[p] = distance;
            }
        }
    }
    float max = std::numeric_limits<float>::lowest();
    for(auto const& distance : minimum_distance) {
        if(distance > max) {
            max = distance;
        }
    }

    return max;
}