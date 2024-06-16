//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_RENDERPASS_HPP
#define MARCHING_CUBES_POINT_CLOUD_RENDERPASS_HPP

#include "vkobject.hpp"
#include "vkimage.hpp"
#include "vkutil.hpp"
#include "vulkan_window.hpp"
#include "render_constants.hpp"
#include "to_string.hpp"

#include <tuple>


labutils::RenderPass create_render_pass( labutils::VulkanWindow const& );
labutils::RenderPass create_imgui_render_pass(labutils::VulkanWindow const&);


std::tuple<labutils::Image, labutils::ImageView> create_depth_buffer(labutils::VulkanWindow const&, labutils::Allocator const&);

labutils::PipelineLayout create_pipeline_layout(labutils::VulkanContext const&, VkDescriptorSetLayout const&);
labutils::Pipeline create_pipeline(labutils::VulkanWindow const &aWindow, VkRenderPass aRenderPass, VkPipelineLayout aPipelineLayout);


#endif //MARCHING_CUBES_POINT_CLOUD_RENDERPASS_HPP
