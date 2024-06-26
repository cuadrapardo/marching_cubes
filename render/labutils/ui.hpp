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

/* General UI helper functions */

namespace ui {
    ImGui_ImplVulkan_InitInfo setup_imgui(labutils::VulkanWindow const&, labutils::DescriptorPool const&);
}

// Describes values of the different ImGui widgets
struct UiConfiguration {
    bool vertices = true, vertex_color = true, distance_field = true, grid = true, edge_color = true, surface = false;
    float grid_resolution = 1.0f;
    const float grid_resolution_min = 1.0f, grid_resolution_max = 2.0f;
    int point_cloud_size = 5;
    const int p_cloud_size_min = 1, p_cloud_size_max = 10;
    int isovalue = 5; //TODO: limit max / min depending on data.
};

// Recalculates grid and scalar values with given UiConfiguration
void recalculate_grid(PointCloud& pointCloud, PointCloud& distanceField,
                      UiConfiguration const& ui_config, BoundingBox const& bbox,
                      std::vector<PointBuffer>& pBuffer, std::vector<LineBuffer>& lineBuffer,
                      labutils::VulkanContext const& window, labutils::Allocator const& allocator);

#endif //MARCHING_CUBES_POINT_CLOUD_UI_HPP
