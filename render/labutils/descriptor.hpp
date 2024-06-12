//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_DESCRIPTOR_HPP
#define MARCHING_CUBES_POINT_CLOUD_DESCRIPTOR_HPP

#include <cstring>

#include "vulkan_window.hpp"
#include "vkobject.hpp"
#include "error.hpp"
#include "to_string.hpp"
#include "allocator.hpp"
#include "../cw1/simple_model.hpp"
#include "../cw1/load_model.hpp"
#include "render_constants.hpp"
#include "vkutil.hpp"


std::vector<TexturedMesh> create_textured_meshes(labutils::VulkanContext const &window, labutils::Allocator const &allocator, SimpleModel& obj);

void update_scene_uniforms(
        glsl::SceneUniform&,
        std::uint32_t aFramebufferWidth,
        std::uint32_t aFramebufferHeight,
        UserState const& state
);

labutils::DescriptorSetLayout create_scene_descriptor_layout( labutils::VulkanWindow const& );
labutils::DescriptorSetLayout create_object_descriptor_layout( labutils::VulkanWindow const& );


#endif //MARCHING_CUBES_POINT_CLOUD_DESCRIPTOR_HPP
