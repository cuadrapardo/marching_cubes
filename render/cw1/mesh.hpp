//
// Created by Carolina Cuadra Pardo on 6/30/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_MESH_HPP
#define MARCHING_CUBES_POINT_CLOUD_MESH_HPP

#include <vector>
#include <glm/vec3.hpp>
#include "../labutils/vkbuffer.hpp"
#include "../labutils/error.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/to_string.hpp"
#include "../../incremental_remeshing/halfedge.hpp"

struct Mesh {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec3> normals;

    Mesh();
    // Constructor that initializes a Mesh from a HalfEdgeMesh
    Mesh(HalfEdgeMesh const& halfEdgeMesh);

    void set_color(glm::vec3 const& color);
    void set_normals(glm::vec3 const& normal); //TODO: remove.



};

struct MeshBuffer {
    labutils::Buffer positions;
    labutils::Buffer colors;
    labutils::Buffer normals;

    std::uint32_t vertexCount;
};


MeshBuffer create_mesh_buffer(Mesh const& mesh, labutils::VulkanContext const& window, labutils::Allocator const& allocator);

#endif //MARCHING_CUBES_POINT_CLOUD_MESH_HPP
