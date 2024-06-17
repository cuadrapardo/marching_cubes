//
// Created by Carolina Cuadra Pardo on 6/17/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_UI_HPP
#define MARCHING_CUBES_POINT_CLOUD_UI_HPP
#include <string>

#include <imgui/backends/imgui_impl_vulkan.h>

#include "vulkan_window.hpp"
#include "vkobject.hpp"

namespace ui {
    ImGui_ImplVulkan_InitInfo setup_imgui(labutils::VulkanWindow const&, labutils::DescriptorPool const&);

}

#endif //MARCHING_CUBES_POINT_CLOUD_UI_HPP
