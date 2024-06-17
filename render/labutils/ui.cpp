//
// Created by Carolina Cuadra Pardo on 6/17/24.
//

#include "ui.hpp"

namespace ui {
    ImGui_ImplVulkan_InitInfo
    setup_imgui(labutils::VulkanWindow const& window, labutils::DescriptorPool const& imguiDpool) {
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = window.instance;
        init_info.PhysicalDevice = window.physicalDevice;
        init_info.Device = window.device;
        init_info.Queue = window.graphicsQueue;
        init_info.DescriptorPool = imguiDpool.handle;
        init_info.MinImageCount = window.swapImages.size();
        init_info.ImageCount = window.swapImages.size();
        init_info.UseDynamicRendering = true;


        //dynamic rendering parameters for imgui to use
        init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &window.swapchainFormat;

        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        return init_info;
    }
}
