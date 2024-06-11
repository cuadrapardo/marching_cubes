//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#include "descriptor.hpp"

labutils::DescriptorSetLayout create_object_descriptor_layout(labutils::VulkanWindow const &aWindow) {
    VkDescriptorSetLayoutBinding bindings[1]{};
    bindings[0].binding = 0; // this must match the shaders
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layoutInfo.pBindings = bindings;

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout); VK_SUCCESS !=
                                                                                                     res) {
        throw labutils::Error("Unable to create descriptor set layout\n"
                         "vkCreateDescriptorSetLayout() returned %s", labutils::to_string(res).c_str());
    }
    return labutils::DescriptorSetLayout(aWindow.device, layout);

}

labutils::DescriptorSetLayout create_scene_descriptor_layout(labutils::VulkanWindow const &aWindow) {
    VkDescriptorSetLayoutBinding bindings[1]{};
    bindings[0].binding = 0; //Number must match the index of the corresponding binding = N declaration in the
    // Shaders!
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layoutInfo.pBindings = bindings;

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout);
            VK_SUCCESS != res) {
        throw labutils::Error("Unable to create descriptor set layoutt\n"
                         "vkCreateDescriptorSetLAyout() returned %s", labutils::to_string(res).c_str());
    }
    return labutils::DescriptorSetLayout(aWindow.device, layout);
}
