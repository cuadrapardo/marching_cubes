//
// Created by Carolina Cuadra Pardo on 6/12/24.
//

#include "point_cloud.hpp"
#include "../labutils/error.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/to_string.hpp"
#include <unordered_set>
#include <algorithm>
#include <cstring>

//Creates buffers to hold position & color information
PointCloud obj_to_pointcloud(SimpleModel& obj_file, labutils::VulkanContext const& window, labutils::Allocator const& allocator ) {
    std::uint32_t positions_size = sizeof(obj_file.dataUntextured.positions[0]) * (obj_file.dataUntextured.positions.size() + obj_file.dataTextured.positions.size());
    std::uint32_t color_size = positions_size;

    std::vector<glm::vec3> positions, colors;
    positions.insert(positions.end(), obj_file.dataTextured.positions.begin(),  obj_file.dataTextured.positions.end());
    positions.insert(positions.end(), obj_file.dataUntextured.positions.begin(),  obj_file.dataUntextured.positions.end());

    //Add color (default to red)
    std::fill(colors.begin(), colors.end(), glm::vec3{1.0f, 0.0f, 0.0f});

    labutils::Buffer vertexPosGPU = labutils::create_buffer(
            allocator,
            positions_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0, //no additional VmaAllocationCreateFlags
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE //or just VMA_MEMORY_USAGE_AUTO
    );

    labutils::Buffer vertexColGPU = labutils::create_buffer(allocator,
                                                            color_size,
                                                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            0,
                                                            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    );

    labutils::Buffer posStaging = labutils::create_buffer(allocator,
                                                          positions_size,
                                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    labutils::Buffer colStaging = labutils::create_buffer(allocator,
                                                          color_size,
                                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    void *posPtr = nullptr;
    if (auto const res = vmaMapMemory(allocator.allocator, posStaging.allocation, &posPtr); VK_SUCCESS !=
                                                                                            res) {
        throw labutils::Error("Mapping memory for writing\n"
                              "vmaMapMemory() returned %s", labutils::to_string(res).c_str());
    }
    std::memcpy(posPtr, positions.data(), positions_size);
    vmaUnmapMemory(allocator.allocator, posStaging.allocation);

    void* colPtr = nullptr;
    if(auto const res = vmaMapMemory(allocator.allocator, colStaging.allocation, &colPtr); VK_SUCCESS != res){
        throw labutils::Error("Mapping memory for writing\n"
                              "vmaMapMemory() returned %s", labutils::to_string(res).c_str());
    }
    std::memcpy(colPtr, colors.data() , color_size);
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
    pcopy.size = positions_size;

    vkCmdCopyBuffer(uploadCmd, posStaging.buffer, vertexPosGPU.buffer, 1, &pcopy);

    labutils::buffer_barrier(uploadCmd,
                             vertexPosGPU.buffer,
                             VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

    VkBufferCopy ccopy{};
    ccopy.size = color_size;

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

    return PointCloud {
        std::move(vertexPosGPU),
        std::move(vertexColGPU)
    };

}
