//
// Created by Carolina Cuadra Pardo on 6/24/24.
//

#include "surface_reconstruction.hpp"
#include "distance_field.hpp"


/* Returns case index in the 256 case table. Important - in order for this to work, the  vertex values
 * passed in need to be in the same order as described in mc_tables.h. */
unsigned int get_case(unsigned int const (&vertex_values)[8]) {
    unsigned int cube_idx = 0;

    //IS THIS RIGHT???
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
//    1 +-----+-------+  5  |         +-----|-11----+     |
//      |   2 +-------+-----+ 6       |     +-----9-|----+
//      |   /         |   /           3   0         7   4
//      | /           | /             | /           | /
//    0 +-------------+ 4             +------8------+
// Diagram inspired by: https://gist.github.com/dwilliamson/c041e3454a713e58baf6e4f8e5fffecd
/* The diagram refers to the layout desribed in mc_tables.h
 * Given the grid values ( 0 or 1 - negative or positive), iterate and for each cube find its case.
 * Returns a vector of points where each triple(3) of vec3s define a triangle */
void query_case_table(std::vector<unsigned int> const& grid_values, float const& grid_resolution, BoundingBox const& model_bbox) {
    //TODO: double check -  are grid values in the same order as vertex index ? are they indexed 1 to 1 ?
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

    for (unsigned int i = 0; i <= grid_boxes.x + 1 ; i++) {
        for(unsigned int j = 0; j <= grid_boxes.y + 1; j++) {
            for(unsigned int k = 0; k <= grid_boxes.z + 1; k++) {

                //Get cube vertices
                unsigned int vertex_values[8];

                vertex_values[0] = grid_values[(get_index(i, j, k))];
                vertex_values[1] = grid_values[(get_index(i, j, k+1))];
                vertex_values[2] = grid_values[(get_index(i, j+1, k))];
                vertex_values[3] = grid_values[(get_index(i, j+1, k+1))];
                vertex_values[4] = grid_values[(get_index(i+1, j, k))];
                vertex_values[5] = grid_values[(get_index(i+1, j, k+1))];
                vertex_values[6] = grid_values[(get_index(i+1, j+1, k))];
                vertex_values[7] = grid_values[(get_index(i+1, j+1, k+1))];

                unsigned int case_index = get_case(vertex_values);
                int case_entry[17];
                std::copy(std::begin(triangleTable[case_index]), std::end(triangleTable[case_index]), case_entry); // ??? DOES THIS WORK? NEVER USED
                unsigned int triangle_n = case_entry[0]; //Number of triangles generated in this case
                if(triangle_n == 0) continue; //Ignore cases which do not generate triangles - ie all positive or negative vertices
            }
        }
    }

}
