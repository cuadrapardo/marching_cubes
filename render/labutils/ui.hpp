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


namespace ui {
    ImGui_ImplVulkan_InitInfo setup_imgui(labutils::VulkanWindow const&, labutils::DescriptorPool const&);

}

void recalculate_grid(PointCloud& pointCloud, PointCloud& distanceField,
                      int const& point_size, float const& grid_resolution,
                      std::vector<PointBuffer>& pBuffer, std::vector<LineBuffer>& lineBuffer,
                      labutils::VulkanContext const& window, labutils::Allocator const& allocator);

#endif //MARCHING_CUBES_POINT_CLOUD_UI_HPP
