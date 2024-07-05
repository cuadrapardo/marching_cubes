//
// Created by Carolina Cuadra Pardo on 6/30/24.
//


#include "test_scene.hpp"
#include "../marching_cubes/mc_tables.h"
#include "../render/cw1/mesh.hpp"
#include "../render/labutils/render_constants.hpp"

#if TEST_MODE == ON
//TODO: add this into premake instead of here.


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

    cube.set_color(glm::vec3{0,0,0});
    cube.set_size(5);

    return cube;
}

/* Classifies edges as positive, bipolar, negative , depending on the value at the endpoints. TODO: fixme, right now all edges are black */
std::pair<std::vector<unsigned int>, std::vector<glm::vec3>> classify_cube_edges(std::vector<unsigned int> const& edge_indices,
        std::vector<unsigned int> const& vertex_values) {
    std::vector<glm::vec3> color;
    std::vector<unsigned int> edge_value;
    for(unsigned int edge = 0; edge < edge_indices.size(); edge++) {
        unsigned int v_0 = edge_indices[edge];
        unsigned int v_1 = edge_indices[edge+1];
        if(vertex_values[v_0] == 0 && vertex_values[v_1] == 0) {
            color.push_back(glm::vec3{0.1,0.01,0.44});
            edge_value.push_back(0);
        }

        if(vertex_values[v_0] == 1 && vertex_values[v_1] == 1) {
            color.push_back(glm::vec3{0.1,0.01,0.44});
            edge_value.push_back(1);
        }

        if(vertex_values[v_0] == 1 && vertex_values[v_1 ] == 0 ||
        vertex_values[v_0] == 0 && vertex_values[v_1] == 1) {
            color.push_back(glm::vec3{0.1,0.01,0.44});
            edge_value.push_back(2);
        }
    }

    return std::make_pair(edge_value, color);
}

/* Returns vector that indexes cube positions to describe edges as two endpoints */
std::vector<unsigned int> get_test_scene_edges() {
    std::vector<unsigned int> edges;
    //Edge 0
    edges.push_back(0);
    edges.push_back(2);

    //Edge 1
    edges.push_back(2);
    edges.push_back(3);

    //Edge 2
    edges.push_back(3);
    edges.push_back(1);

    //Edge 3
    edges.push_back(1);
    edges.push_back(0);

    //Edge 4
    edges.push_back(4);
    edges.push_back(6);

    //Edge 5
    edges.push_back(6);
    edges.push_back(7);

    //Edge 6
    edges.push_back(7);
    edges.push_back(5);

    //Edge 7
    edges.push_back(5);
    edges.push_back(4);

    //Edge 8
    edges.push_back(4);
    edges.push_back(0);

    //Edge 9
    edges.push_back(2);
    edges.push_back(6);

    //Edge 10
    edges.push_back(3);
    edges.push_back(7);

    //Edge 11
    edges.push_back(1);
    edges.push_back(5);

    return edges;
}




/* Recalculate the test scene by changing the colours depending on the vertex values,
 * and recalculate the MC surface created. */
void recalculate_test_scene(PointCloud& cube, Mesh& triangles, std::vector<unsigned int> const& cube_edges, std::vector<unsigned int> const& vertex_values,
                            std::vector<PointBuffer>& pBuffer, std::vector<LineBuffer>& lBuffer, std::vector<MeshBuffer>& mBuffer,
                            labutils::VulkanContext const& window, labutils::Allocator const& allocator) {
    triangles.positions.clear();
    triangles.normals.clear();


    //Recalculate vertex colors
    for(unsigned int vertex = 0; vertex < 8 ; vertex++) {
        cube.colors[vertex] = (vertex_values[vertex] == 0) ? glm::vec3{0,0,0} : glm::vec3{1,1,1};
    }

    //Recalculate edge colors
    auto [cube_edge_values, cube_edge_colors] = classify_cube_edges(cube_edges, vertex_values);
//    std::vector<glm::vec3> cube_edge_colors;
//    for (int i = 0; i < ; +i) {
//
//    }

    //Create buffers for rendering
    PointBuffer cubeBuffer = create_pointcloud_buffers(cube.positions, cube.colors, cube.point_size,
                                                       window, allocator);
    LineBuffer lineBuffer = create_index_buffer(cube_edges, cube_edge_colors, window, allocator);

    Mesh case_triangles;
    case_triangles.positions =  query_case_table_test(vertex_values, cube.positions, TEST_ISOVALUE);

    case_triangles.set_normals(glm::vec3{1, 1, 0});
    case_triangles.set_color(glm::vec3{1, 0, 0});

    if(!case_triangles.positions.empty()) { // Do not create an empty buffer - this will produce an error.
        if(mBuffer.size() > 0) {
            // Destroy buffers. In the case of the test scene, the buffers will not change size for points and lines (since we don't change the resolution)
            // A better approach would be to simply swap the values in the buffers.
            mBuffer[0].vertexCount = 0;
            MeshBuffer test_b = create_mesh_buffer(case_triangles, window, allocator);
            mBuffer[0] = std::move(test_b);
        } else {
            MeshBuffer test_b = create_mesh_buffer(case_triangles, window, allocator);
            mBuffer.push_back(std::move(test_b));
        }
    } else {
        mBuffer[0].vertexCount = 0;
    }

    pBuffer[0] = (std::move(cubeBuffer));
    lBuffer[0] = (std::move(lineBuffer));

    std::cout << "Continue rendering with new buffers" << std::endl;

}

#endif