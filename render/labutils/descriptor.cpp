//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#include "descriptor.hpp"

void update_scene_uniforms( glsl::SceneUniform& aSceneUniforms, std::uint32_t aFramebufferWidth, std::uint32_t aFramebufferHeight, UserState const& aState)
{
    float const aspect = aFramebufferWidth / float(aFramebufferHeight);

    aSceneUniforms.projection = glm::perspectiveRH_ZO(
            labutils::Radians(cfg::kCameraFov).value(),
            aspect,
            cfg::kCameraNear,
            cfg::kCameraFar
    );
    aSceneUniforms.projection[1][1] *= -1.f; //Mirror y axis
    aSceneUniforms.camera = glm::translate(glm::vec3(0.f, -0.3f, -1.f));
    aSceneUniforms.camera = aSceneUniforms.camera * glm::inverse(aState.camera2world);

    aSceneUniforms.projCam = aSceneUniforms.projection * aSceneUniforms.camera;

}

std::vector<TexturedMesh> create_textured_meshes(labutils::VulkanContext const& window, labutils::Allocator const& allocator, SimpleModel& obj ){
    std::vector<TexturedMesh>  texturedMeshes;
    for (unsigned int mesh = 0; mesh < obj.meshes.size(); ++mesh) {
        SimpleMeshInfo meshInfo = obj.meshes[mesh];
        SimpleMaterialInfo matInfo = obj.materials[meshInfo.materialIndex];

        std::vector<glm::vec3> meshPositions;
        meshPositions.reserve(meshInfo.vertexCount);
        std::vector<glm::vec2> meshTexcoords;
        std::vector<glm::vec3> meshColor;
        meshColor.reserve(meshInfo.vertexCount); //Same as this, can possibly just be a colour (single glm::vec3)

        std::string pathToTex;
        if (meshInfo.textured) {
            meshTexcoords.reserve(meshInfo.vertexCount);
            std::vector<glm::vec3>::const_iterator first =
                    obj.dataTextured.positions.begin() + meshInfo.vertexStartIndex;
            std::vector<glm::vec3>::const_iterator last =
                    obj.dataTextured.positions.begin() + meshInfo.vertexStartIndex + meshInfo.vertexCount ;
            meshPositions.assign(first, last);
            std::vector<glm::vec2>::const_iterator firstTex =
                    obj.dataTextured.texcoords.begin() + meshInfo.vertexStartIndex;
            std::vector<glm::vec2>::const_iterator lastTex =
                    obj.dataTextured.texcoords.begin() + meshInfo.vertexStartIndex + meshInfo.vertexCount;
            meshTexcoords.assign(firstTex, lastTex);


            glm::vec3 color(1,1,1);
            meshColor.assign(meshInfo.vertexCount, color);

            pathToTex = matInfo.diffuseTexturePath;


        } else { //Untextured
            std::vector<glm::vec3>::const_iterator first =
                    obj.dataUntextured.positions.begin() + meshInfo.vertexStartIndex;
            std::vector<glm::vec3>::const_iterator last =
                    obj.dataUntextured.positions.begin() + meshInfo.vertexStartIndex + meshInfo.vertexCount ;
            meshPositions.assign(first, last);

            glm::vec2 const texCoordinate = {0, 1};
            meshTexcoords.push_back(texCoordinate); //Only need one texcoordinate to map a texture unsure

            meshColor.assign(meshInfo.vertexCount, matInfo.diffuseColor);
            pathToTex = WHITE_MAT;

        }

        std::uint32_t positionsSize = sizeof(meshPositions[0]) * meshPositions.size();
        std::uint32_t texcoordsSize = sizeof(meshTexcoords[0]) * meshTexcoords.size();
        std::uint32_t colorSize = sizeof(meshColor[0]) * meshColor.size();

        labutils::Buffer vertexPosGPU = labutils::create_buffer(
                allocator,
                positionsSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                0, //no additional VmaAllocationCreateFlags
                VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE //or just VMA_MEMORY_USAGE_AUTO
        );
        labutils::Buffer vertexTexGPU = labutils::create_buffer(allocator,
                                                      texcoordsSize,
                                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                      0,
                                                      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        );
        labutils::Buffer vertexColGPU = labutils::create_buffer(allocator,
                                                      colorSize,
                                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                      0,
                                                      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        );
        labutils::Buffer posStaging = labutils::create_buffer(allocator,
                                                    positionsSize,
                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        labutils::Buffer texStaging = labutils::create_buffer(allocator,
                                                    texcoordsSize,
                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        labutils::Buffer colStaging = labutils::create_buffer(allocator,
                                                    colorSize,
                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        void *posPtr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, posStaging.allocation, &posPtr); VK_SUCCESS !=
                                                                                                res) {
            throw labutils::Error("Mapping memory for writing\n"
                             "vmaMapMemory() returned %s", labutils::to_string(res).c_str());
        }
        std::memcpy(posPtr, meshPositions.data(), positionsSize);
        vmaUnmapMemory(allocator.allocator, posStaging.allocation);

        void *texPtr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, texStaging.allocation, &texPtr); VK_SUCCESS !=
                                                                                                res) {
            throw labutils::Error("Mapping memory for writing\n"
                             "vmaMapMemory() returned %s", labutils::to_string(res).c_str());
        }
        std::memcpy(texPtr, meshTexcoords.data(), texcoordsSize);
        vmaUnmapMemory(allocator.allocator, texStaging.allocation);

        void* colPtr = nullptr;
        if(auto const res = vmaMapMemory(allocator.allocator, colStaging.allocation, &colPtr); VK_SUCCESS != res){
            throw labutils::Error("Mapping memory for writing\n"
                             "vmaMapMemory() returned %s", labutils::to_string(res).c_str());
        }
        std::memcpy(colPtr, meshColor.data() , colorSize);
        vmaUnmapMemory(allocator.allocator, colStaging.allocation);

        //We need to ensure that the Vulkan resources are alive until all the transfers have completed. For simplicity,
        //we will just wait for the operations to complete with a fence. A more complex solution might want to queue
        //transfers, let these take place in the background while performing other tasks.
        labutils::Fence uploadComplete = create_fence(window);

        //Queue data uploads from staging buffers to the final buffers.
        //This uses a separate command pool for simplicity.
        labutils::CommandPool uploadPool = create_command_pool(window);
        VkCommandBuffer uploadCmd = alloc_command_buffer(window, uploadPool.handle);

        //Record the copy commands into the command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(uploadCmd, &beginInfo); VK_SUCCESS != res) {
            throw labutils::Error("Beginning command buffer recording\n"
                             "vkBeginCommandBuffer() returned %s", labutils::to_string(res).c_str());
        }

        VkBufferCopy pcopy{};
        pcopy.size = positionsSize;

        vkCmdCopyBuffer(uploadCmd, posStaging.buffer, vertexPosGPU.buffer, 1, &pcopy);

        labutils::buffer_barrier(uploadCmd,
                            vertexPosGPU.buffer,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

        VkBufferCopy tcopy{};
        tcopy.size = texcoordsSize;

        vkCmdCopyBuffer(uploadCmd, texStaging.buffer, vertexTexGPU.buffer, 1, &tcopy);

        labutils::buffer_barrier(uploadCmd,
                            vertexTexGPU.buffer,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
        );

        VkBufferCopy ccopy{};
        ccopy.size = colorSize;

        vkCmdCopyBuffer(uploadCmd, colStaging.buffer, vertexColGPU.buffer, 1, &ccopy);

        labutils::buffer_barrier(uploadCmd,
                            vertexColGPU.buffer,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
        );
        if(auto const res = vkEndCommandBuffer(uploadCmd);VK_SUCCESS != res) {
            throw labutils::Error("Ending command buffer recording\n"
                             "vkEndCommandBUffer() returned %s", labutils::to_string(res).c_str());
        }

        //Submit transfer commands
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &uploadCmd;

        if (auto const res = vkQueueSubmit(window.graphicsQueue, 1, &submitInfo, uploadComplete.handle);
                VK_SUCCESS !=
                res) {
            throw labutils::Error("Submitting commands\n"
                             "vkQueueSubmit() returned %s", labutils::to_string(res).c_str());
        }

        //Wait for commands to finish before we destroy the temporary resources required for the transfers
        // ( staging buffers, command pool,...)
        //
        //The code doesn't destroy the resources implicitly. The resources are destroyed by the destructors of the labutils
        //wrappers for the various objects once we leave the function's scope.

        if (auto const res = vkWaitForFences(window.device, 1, &uploadComplete.handle, VK_TRUE,
                                             std::numeric_limits<std::uint64_t>::max());
                VK_SUCCESS != res) {
            throw labutils::Error("Waiting for upload to complete\n"
                             "vkWaitForFences() returned %s", labutils::to_string(res).c_str());
        }


        TexturedMesh m{
                std::move(vertexPosGPU),
                std::move(vertexTexGPU),
                std::move(vertexColGPU),
                pathToTex,
                static_cast<uint32_t>(positionsSize / sizeof(float) / 3),
        };

        texturedMeshes.emplace_back(std::move(m));
    }
    return texturedMeshes;
}

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
