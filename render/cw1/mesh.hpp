//
// Created by Carolina Cuadra Pardo on 6/30/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_MESH_HPP
#define MARCHING_CUBES_POINT_CLOUD_MESH_HPP

#include <vector>
#include <unordered_map>
#include <glm/vec3.hpp>
#include "../labutils/vkbuffer.hpp"
#include "../labutils/error.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/to_string.hpp"
#include "../../incremental_remeshing/halfedge.hpp"


template<>
struct std::hash<glm::vec3> {
    std::size_t operator()(const glm::vec3& v) const {
        return ((std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 1)) >> 1) ^ (std::hash<float>()(v.z) << 1);
    }
};

struct IndexedMesh {
    std::unordered_map<glm::vec3, int> vertex_idx_map;
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> face_indices;


};

struct Mesh {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec3> normals;

    Mesh();
    // Constructor that initializes a Mesh from a HalfEdgeMesh
    Mesh(HalfEdgeMesh const& halfEdgeMesh);
    Mesh(IndexedMesh const& indexedMesh);

    void set_color(glm::vec3 const& color);
    void set_normals(glm::vec3 const& normal);



};







struct MeshBuffer {
    labutils::Buffer positions;
    labutils::Buffer colors;
    labutils::Buffer normals;

    std::uint32_t vertexCount;
};


MeshBuffer create_mesh_buffer(Mesh const& mesh, labutils::VulkanContext const& window, labutils::Allocator const& allocator);

#endif //MARCHING_CUBES_POINT_CLOUD_MESH_HPP
