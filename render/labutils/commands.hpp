//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_COMMANDS_HPP
#define MARCHING_CUBES_POINT_CLOUD_COMMANDS_HPP

#include <volk.h>
#include <vector>

#include "render_constants.hpp"
#include "vulkan_window.hpp"
#include "vkutil.hpp"
#include "to_string.hpp"
#include "../cw1/point_cloud.hpp"

#include <imgui/backends/imgui_impl_vulkan.h>


void record_commands_textured( VkCommandBuffer,
                               VkRenderPass,
                               VkFramebuffer,
                               VkPipeline,
                               VkExtent2D const&,
                               VkBuffer aSceneUBO,
                               glsl::SceneUniform const&,
                               VkPipelineLayout,
                               VkDescriptorSet sceneDescriptors,
                               PointCloud& pCloud);

namespace ui {
    void record_commands_imgui(
            VkCommandBuffer cbufferImgui,
            labutils::VulkanWindow const& ,
            unsigned int const& imageIndex
    );
}

void submit_commands(
        labutils::VulkanWindow const&,
        VkCommandBuffer,
        VkFence,
        VkSemaphore,
        VkSemaphore
);


#endif //MARCHING_CUBES_POINT_CLOUD_COMMANDS_HPP
