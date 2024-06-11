//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_DESCRIPTOR_HPP
#define MARCHING_CUBES_POINT_CLOUD_DESCRIPTOR_HPP

#include "vulkan_window.hpp"
#include "vkobject.hpp"
#include "error.hpp"
#include "to_string.hpp"



labutils::DescriptorSetLayout create_scene_descriptor_layout( labutils::VulkanWindow const& );
labutils::DescriptorSetLayout create_object_descriptor_layout( labutils::VulkanWindow const& );


#endif //MARCHING_CUBES_POINT_CLOUD_DESCRIPTOR_HPP
