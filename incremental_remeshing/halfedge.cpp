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

/* Given a vertex idx, deletes it from: vertex_position, vertex_normal
void HalfEdgeMesh::delete_vertex(const unsigned int& vertex_idx) {

} ??? Might not be a good idea to implement- t*/

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

    //Get he of index 0 within this face (first halfedge of the face)
    auto halfedges = get_halfedges(face_idx_0);

    //Delete halfedges
    for(auto const& halfedge : halfedges ) {
        halfedges_opposite.erase(halfedges_opposite.begin() + halfedge );
        halfedges_vertex_to.erase(halfedges_vertex_to.begin() + halfedge );
        //Note: opposites are not deleted because they need to be reconnected later on. Those trianlges are not destroyed.
    }

    //Shift all values above halfedge 2 (last halfedge)
    for(unsigned int he = 0; he < halfedges_opposite.size(); he++) {
        if(halfedges_opposite[he] > halfedges[2]) {
            halfedges_opposite[he] -= 3; //Deleting 3 halfedges, so shift by 3.
        }

    }
    for(auto& outgoing_he : vertex_outgoing_halfedge) {
        if(outgoing_he > halfedges[2]) {
            outgoing_he -= 3; //Deleting 3 halfedges, so shift by 3.
        }
    }

    //Update values of other halves if they were shifted
    he_next_oh = (he_next_oh > halfedges[2]) ? he_next_oh -3 : he_next_oh;
    he_prev_oh = (he_prev_oh > halfedges[2]) ? he_prev_oh -3 : he_prev_oh;

    //Reset connectivity of other halves belonging to edges in the deleted triangle
    halfedges_opposite[he_next_oh] = he_prev_oh;
    halfedges_opposite[he_prev_oh] = he_next_oh;

    //Reset first directed edges from collapsed triangle
    vertex_outgoing_halfedge[non_edge_vertex] = he_next_oh;
}


/* Iterates through one ring and returns vertex indices of those belonging to the given vertex's one ring */
std::unordered_set<unsigned int> HalfEdgeMesh::get_one_ring_vertices(const unsigned int& vertex_idx) {
    std::unordered_set<unsigned int> one_ring;
    //Get first directed edge
    int const& fde = vertex_outgoing_halfedge[vertex_idx];
    one_ring.insert(halfedges_vertex_to[fde]);
    int current_he_idx = get_previous_halfedge(fde);
    assert(halfedges_vertex_to[current_he_idx] == vertex_idx);
    while(fde != current_he_idx) { //ISSUE: infinite loop
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
//TODO: check if boundary. Right now it is okay to not check as the input is assumed to be from Marching Cubes application
void HalfEdgeMesh::edge_collapse(const unsigned int& he_idx, const float& high_edge_length) {
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
        return;
    }

    // Check if all elements are equal (they must be), if not it must mean there are more triangles connecting to p and q which
    // do not form a triangle with edge pq
    for (const int& vertex : intersection) {
        if (connected_vertices.find(vertex) == connected_vertices.end()) {
            return;
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
                return; //Collapsing he_idx will create longer edges.
            }
        }
    }

    //Update data structure to reflect edge collapse
    vertex_outgoing_halfedge[vertex_to] = halfedges_opposite[get_previous_halfedge(halfedges_opposite[he_idx])]; //To avoid clashes, set the fde to an edge which will NOT get collapsed.

    //Remove two triangles adjacent to the edge
    //Triangle 0 - triangle belonging to the other half of the edge collapsed
    auto halfedges = get_halfedges(get_face(he_idx));
    delete_face(he_idx);

    //Triangle 1 - triangle belonging to the other half of the edge collapsed
    auto updated_he_opposite_idx = ( he_opposite_idx > halfedges[2]) ? he_opposite_idx - 3 : he_opposite_idx;
    delete_face(updated_he_opposite_idx);


    //Remove vertex
    vertex_positions.erase(vertex_positions.begin() + vertex_from); //Indices of all vertices are therefore changed by -1 if of a higher idx than vertex_from
    vertex_outgoing_halfedge.erase(vertex_outgoing_halfedge.begin() + vertex_from);


    //TODO: update vertex normals.

    //Update halfedges pointing to the deleted vertex.
    for(unsigned int halfedge = 0; halfedge < halfedges_vertex_to.size(); halfedge++) {
        int& curr_vertex_to = halfedges_vertex_to[halfedge];
        if(curr_vertex_to == vertex_from) {
            curr_vertex_to = vertex_to;
        }
        if(curr_vertex_to > vertex_from) {
            curr_vertex_to--;
        }
    }


    //Update outgoing halfedge for vertices that might have had ng into account removed halfedges
    for(unsigned int outgoing_he = 0; outgoing_he < vertex_outgoing_halfedge.size(); outgoing_he++) {

    }


    //Update face indices - all indices bigger than deleted vertex are shifted by -1
    //                    - Triangles that have the deleted vertex now have the vertex at the other side of the edge (vertex_to)
    for(unsigned int face = 0; face < faces.size(); face +=3) {
        for(unsigned int vertex_offset = 0; vertex_offset < 3; vertex_offset++) {
            unsigned int idx = face + vertex_offset;
            unsigned int& vertex_idx = faces[idx];
            if (vertex_idx == vertex_from) { //Replace deleted vertex TODO: maybe don't do this for to be deleted triangles???
                vertex_idx = vertex_to;
                continue;
            }
            if(vertex_idx > vertex_from) { //Shift indices
                vertex_idx--;
            }
        }
    }

    //CONNECTIVITY IS BROKEN

}

/* Splits all edges longer than high_edge_length at their midpoint */
void HalfEdgeMesh::split_long_edges(const float& high_edge_length) {
    std::cout << "Splitting edges longer than " << high_edge_length << std::endl;
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
    for(unsigned int edge = 0; edge < halfedges_vertex_to.size(); edge++) { // IMPORTANT: I will be modifying this as I iterate.
        if(get_edge_length(edge) >= low_edge_length) { continue; }
        edge_collapse(edge, high_edge_length);
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
void HalfEdgeMesh::remesh(float const& input_target_edge_length) {
    float target_edge_length;
    if(input_target_edge_length == 0) {
        target_edge_length = get_mean_edge_length();
    } else {
        target_edge_length = input_target_edge_length;
    }
    float low = 4/5 * target_edge_length; // the thresholds 4/5 and 4/3
    float high = 4/3 * target_edge_length; // are essential to converge to a uniform edge length

    split_long_edges(high);

    collapse_short_edges(high, low);


}
