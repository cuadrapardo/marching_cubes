//
// Created by Carolina Cuadra Pardo on 6/30/24.
//

#include "test_scene.hpp"

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

/* Populates "pointcloud" - a simple cube with 8 vertices */
PointCloud create_test_scene() {
    PointCloud cube;
    cube.positions.push_back(glm::vec3{0,0,0});
    cube.positions.push_back(glm::vec3{0,0,1});
    cube.positions.push_back(glm::vec3{0,1,0});
    cube.positions.push_back(glm::vec3{0,1,1});
    cube.positions.push_back(glm::vec3{1,0,0});
    cube.positions.push_back(glm::vec3{1,0,1});
    cube.positions.push_back(glm::vec3{1,1,0});
    cube.positions.push_back(glm::vec3{1,1,1});

    cube.set_color(glm::vec3{1,1,1});
    cube.set_size(1);

    return cube;
}

/* Recalculate the test scene by changing the colours depending on the vertex values,
 * and recalculate the MC surface created */
void recalculate_test_scene(unsigned int const (&vertex_configuration)[8], PointCloud& test_scene) {
    //Recalculate vertex colors
    for(unsigned int vertex = 0; vertex < 8 ; vertex++) {
        test_scene.colors[vertex] = (vertex_configuration == 0) ? glm::vec3{0,0,0} : glm::vec3{1,1,1};
    }

    //TODO Recalculate surface

}