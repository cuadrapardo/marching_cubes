//
// Created by Carolina Cuadra Pardo on 6/24/24.
//

#include "surface_reconstruction.hpp"
#include "distance_field.hpp"
#include "mc_tables.h"
#include "../render/labutils/render_constants.hpp"



/* Returns case index in the 256 case table. Important - in order for this to work, the  vertex values
 * passed in need to be in the same order as described in mc_tables.h. */
unsigned int get_case(unsigned int const (&vertex_values)[8]) {
    unsigned int cube_idx = 0;

    if (vertex_values[0] == 1) cube_idx |= 1;
    if (vertex_values[1] == 1) cube_idx |= 2;
    if (vertex_values[2] == 1) cube_idx |= 4;
    if (vertex_values[3] == 1) cube_idx |= 8;
    if (vertex_values[4] == 1) cube_idx |= 16;
    if (vertex_values[5] == 1) cube_idx |= 32;
    if (vertex_values[6] == 1) cube_idx |= 64;
    if (vertex_values[7] == 1) cube_idx |= 128;

    return cube_idx;
}

/* Given two points on the edge where a vertex should be inserted according to the case tables,
 * use their scalar values and positions to interpolate the triangle's vertex.
 * ----> Returns interpolated vertex position between two points according to scalar value */
glm::vec3 linear_interpolation(glm::vec3 const& point_1, glm::vec3 const& point_2,
                               unsigned int const& scalar_1, unsigned int const& scalar_2, float const& isovalue) {
    glm::vec3 interpolated_position;

    float alpha = (isovalue - (float)scalar_1) / ((float)scalar_2 - scalar_1);

    alpha = glm::clamp(alpha, 0.0f, 1.0f); // ??? Double check with hamish that this is appropriate.

    interpolated_position.x = (1-alpha)*point_1.x + (alpha*point_2.x);
    interpolated_position.y = (1-alpha)*point_1.y + (alpha*point_2.y);
    interpolated_position.z = (1-alpha)*point_1.z + (alpha*point_2.z);
    return interpolated_position;
}

//  Axes are:
//
//      z
//      |     y
//      |   /
//      | /
//      +----- x
//
// Vertex and edge layout:
//
//            3             7
//            +-------------+               +-----10------+
//          / |           / |             / |            /|
//        /   |         /   |           2   1          6  5
//    1 +-----|-------+  5  |         +-----|-11----+     |
//      |   2 +-------+-----+ 6       |     +-----9-|----+
//      |   /         |   /           3   0         7   4
//      | /           | /             | /           | /
//    0 +-------------+ 4             +------8------+
// Diagram inspired by: https://gist.github.com/dwilliamson/c041e3454a713e58baf6e4f8e5fffecd
/* The diagram refers to the layout desribed in mc_tables.h
 * Given the grid values ( 0 or 1 - negative or positive), iterate and for each cube find its case.
 * Returns a vector of points where each triple(3) of vec3s define a triangle */
std::vector<glm::vec3> query_case_table(std::vector<unsigned int> const& grid_classification,  std::vector<glm::vec3> const& grid_positions,
                                        std::vector<int> const& grid_scalar_values, float const& grid_resolution, BoundingBox const& model_bbox,
                                        float const& input_isovalue) {
    std::vector<glm::vec3> reconstructed_mesh;
     float isovalue = input_isovalue + 0.5; //TODO: Might be nicer to shift this elsewhere.
    std::cout << "Classifying all cubes in the grid" << std::endl;
    glm::vec3 extents = glm::abs(model_bbox.max - model_bbox.min);
    float scale = 1.0f / grid_resolution;

    glm::ivec3 grid_boxes = {
            extents.x / scale,
            extents.y / scale,
            extents.z / scale,
    };

    auto get_index = [grid_boxes](unsigned int i, unsigned int j, unsigned int k) -> unsigned int {
        return i * (grid_boxes.y + 2) * (grid_boxes.z + 2) + j * (grid_boxes.z + 2) + k;
    }; // note this will only work in a loop structured like the one below

    for (unsigned int i = 0; i <= grid_boxes.x ; i++) {
        for(unsigned int j = 0; j <= grid_boxes.y; j++) {
            for(unsigned int k = 0; k <= grid_boxes.z; k++) {
                //Get cube vertices
                unsigned int vertex_values[8];
                unsigned int vertex_idx[8];

                vertex_idx[0] = get_index(i, j, k);
                vertex_idx[1] = get_index(i, j+1, k);
                vertex_idx[2] = get_index(i, j, k+1);
                vertex_idx[3] = get_index(i, j+1, k+1);
                vertex_idx[4] = get_index(i+1, j, k);
                vertex_idx[5] = get_index(i+1, j+1, k);
                vertex_idx[6] = get_index(i+1, j, k+1);
                vertex_idx[7] = get_index(i+1, j+1, k+1);

                vertex_values[0] = grid_classification[vertex_idx[0]];
                vertex_values[1] = grid_classification[vertex_idx[1]];
                vertex_values[2] = grid_classification[vertex_idx[2]];
                vertex_values[3] = grid_classification[vertex_idx[3]];
                vertex_values[4] = grid_classification[vertex_idx[4]];
                vertex_values[5] = grid_classification[vertex_idx[5]];
                vertex_values[6] = grid_classification[vertex_idx[6]];
                vertex_values[7] = grid_classification[vertex_idx[7]];

                unsigned int case_index = get_case(vertex_values);
                int case_entry[17];
                std::copy(std::begin(triangleTable[case_index]), std::end(triangleTable[case_index]), case_entry);
                unsigned int triangle_n = case_entry[0]; //Number of triangles generated in this case
                if(triangle_n == 0) continue; //Ignore cases which do not generate triangles - ie all positive or negative vertices

                for(unsigned int triplet_count = 1; triplet_count < triangle_n*3; triplet_count += 3) {
                    //Lookup edge vertices in table
                    int triangle_edges[3] = {case_entry[triplet_count],
                                             case_entry[triplet_count+1],
                                             case_entry[triplet_count+2]}; // Columns 1-15 are broken into 5 triplets, each triplet representing
                                                                        // 3 edges of a triangle which can be looked up in the edge table to find the vertices.
                    // Get cube vertex indices
                    int edge_0[2] =  {edgeTable[triangle_edges[0]][0], edgeTable[triangle_edges[0]][1] }; // An edge is described as the two vertices at the end
                    int edge_1[2] =  {edgeTable[triangle_edges[1]][0], edgeTable[triangle_edges[1]][1] };
                    int edge_2[2] =  {edgeTable[triangle_edges[2]][0], edgeTable[triangle_edges[2]][1] };

                    //Interpolate edges to form a triangle
                    unsigned int const& edge_0_vertex_0_idx = vertex_idx[edge_0[0]];
                    unsigned int const& edge_0_vertex_1_idx = vertex_idx[edge_0[1]];

                    unsigned int const& edge_1_vertex_0_idx = vertex_idx[edge_1[0]];
                    unsigned int const& edge_1_vertex_1_idx = vertex_idx[edge_1[1]];

                    unsigned int const& edge_2_vertex_0_idx = vertex_idx[edge_2[0]];
                    unsigned int const& edge_2_vertex_1_idx = vertex_idx[edge_2[1]];

                    glm::vec3 vertex_0 = linear_interpolation(grid_positions[edge_0_vertex_0_idx], grid_positions[edge_0_vertex_1_idx],
                                         grid_scalar_values[edge_0_vertex_0_idx], grid_scalar_values[edge_0_vertex_1_idx], isovalue);

                    glm::vec3 vertex_1 = linear_interpolation(grid_positions[edge_1_vertex_0_idx], grid_positions[edge_1_vertex_1_idx],
                                                              grid_scalar_values[edge_1_vertex_0_idx], grid_scalar_values[edge_1_vertex_1_idx], isovalue);

                    glm::vec3 vertex_2 = linear_interpolation(grid_positions[edge_2_vertex_0_idx], grid_positions[edge_2_vertex_1_idx],
                                                              grid_scalar_values[edge_2_vertex_0_idx], grid_scalar_values[edge_2_vertex_1_idx],  isovalue);
                    reconstructed_mesh.push_back(vertex_0);
                    reconstructed_mesh.push_back(vertex_1);
                    reconstructed_mesh.push_back(vertex_2);

                }

            }
        }
    }
    return reconstructed_mesh;

}

#if TEST_MODE == ON
std::vector<glm::vec3> query_case_table_test(std::vector<unsigned int> const& grid_values, std::vector<glm::vec3> const& grid_positions,
                                             float const& input_isovalue) {
    std::vector<glm::vec3> reconstructed_mesh;
    float isovalue = input_isovalue + 0.5;

    std::cout << "Classifying all cubes in the grid" << std::endl;
    unsigned int vertex_values[8];

    vertex_values[0] = grid_values[0];
    vertex_values[1] = grid_values[1];
    vertex_values[2] = grid_values[2];
    vertex_values[3] = grid_values[3];
    vertex_values[4] = grid_values[4];
    vertex_values[5] = grid_values[5];
    vertex_values[6] = grid_values[6];
    vertex_values[7] = grid_values[7];

    unsigned int case_index = get_case(vertex_values);
    int case_entry[17];
    std::copy(std::begin(triangleTable[case_index]), std::end(triangleTable[case_index]),
              case_entry);

    unsigned int triangle_n = case_entry[0]; //Number of triangles generated in this case
    //Ignore cases which do not generate triangles - ie all positive or negative vertices
    if (triangle_n == 0) {
        std::cout << "No triangles generated";
        return reconstructed_mesh;
    }

    for (unsigned int triplet_count = 1 ;triplet_count < triangle_n*3; triplet_count += 3) {
        //Lookup edge vertices in table
        int triangle_edges[3] = {case_entry[triplet_count],
                                 case_entry[triplet_count + 1],
                                 case_entry[triplet_count +
                                            2]}; // Columns 1-15 are broken into 5 triplets, each triplet representing
        // 3 edges of a triangle which can be looked up in the edge table to find the vertices.

        // Get cube vertex indices
        int edge_0[2] = {edgeTable[triangle_edges[0]][0],
                         edgeTable[triangle_edges[0]][1]}; // An edge is described as the two vertices at the end
        int edge_1[2] = {edgeTable[triangle_edges[1]][0], edgeTable[triangle_edges[1]][1]};
        int edge_2[2] = {edgeTable[triangle_edges[2]][0], edgeTable[triangle_edges[2]][1]};

        //Interpolate edges to form a triangle
        glm::vec3 vertex_0 = linear_interpolation(grid_positions[edge_0[0]],
                                                  grid_positions[edge_0[1]],
                                                  MAP_CLASSIFICATION_TO_VALUE(grid_values[edge_0[0]]),
                                                  MAP_CLASSIFICATION_TO_VALUE(grid_values[edge_0[1]]), isovalue);

        glm::vec3 vertex_1 = linear_interpolation(grid_positions[edge_1[0]],
                                                  grid_positions[edge_1[1]],
                                                  MAP_CLASSIFICATION_TO_VALUE(grid_values[edge_1[0]]),
                                                  MAP_CLASSIFICATION_TO_VALUE(grid_values[edge_1[1]]), isovalue);

        glm::vec3 vertex_2 = linear_interpolation(grid_positions[edge_2[0]],
                                                  grid_positions[edge_2[1]],
                                                  MAP_CLASSIFICATION_TO_VALUE(grid_values[edge_2[0]]),
                                                  MAP_CLASSIFICATION_TO_VALUE(grid_values[edge_2[1]]), isovalue);
        reconstructed_mesh.push_back(vertex_0); // ??? winding order????!
        reconstructed_mesh.push_back(vertex_1);
        reconstructed_mesh.push_back(vertex_2);

    }
    return reconstructed_mesh;
}
#endif


