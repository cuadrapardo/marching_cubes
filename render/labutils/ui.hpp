//
// Created by Carolina Cuadra Pardo on 6/17/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_UI_HPP
#define MARCHING_CUBES_POINT_CLOUD_UI_HPP
#include <string>

#include <imgui/backends/imgui_impl_vulkan.h>

#include "vulkan_window.hpp"
#include "vkobject.hpp"
#include "../cw1/point_cloud.hpp"
#include "../../marching_cubes/distance_field.hpp"
#include "../cw1/mesh.hpp"

/* General UI helper functions */

namespace ui {
    ImGui_ImplVulkan_InitInfo setup_imgui(labutils::VulkanWindow const&, labutils::DescriptorPool const&);
}

// Describes values of the different widgets
struct UiConfiguration {
    bool vertices = true, vertex_color = true, distance_field = true, grid = true, edge_color = true, mc_surface = false, remeshed_surface = true;
    float grid_resolution = 1.0f;
    float padding = 0.75f;
    const float grid_resolution_min = 1.0f, grid_resolution_max = 2.0f;
    int point_cloud_size = 5;
    const int p_cloud_size_min = 1, p_cloud_size_max = 10;
    int isovalue = 2; //TODO: limit max / min depending on data.
    float target_edge_length = 0.0f;
    int remeshing_iterations = 10;

    bool mc_manifold = false, remesh_manifold = false;
    bool flyCamera = true;

    //Mesh Metrics: Hausdorff distance
    float p_cloud_to_MC_mesh;
    float MC_mesh_to_remeshed;

};

// Recalculates grid and scalar values with given UiConfiguration
IndexedMesh recalculate_grid(PointCloud& pointCloud, PointCloud& distanceField, Mesh& triangles,
                      UiConfiguration& ui_config, BoundingBox& bbox,
                      std::vector<PointBuffer>& pBuffer, std::vector<LineBuffer>& lineBuffer, std::vector<MeshBuffer>& mBuffer,
                      labutils::VulkanContext const& window, labutils::Allocator const& allocator);

HalfEdgeMesh recalculate_remeshed_mesh(UiConfiguration& ui_config, labutils::VulkanContext const& window, labutils::Allocator const& allocator,std::vector<MeshBuffer>& mBuffer);

#endif //MARCHING_CUBES_POINT_CLOUD_UI_HPP
