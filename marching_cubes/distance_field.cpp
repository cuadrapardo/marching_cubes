//
// Created by Carolina Cuadra Pardo on 6/17/24.
//

#include "distance_field.hpp"
#include "../third_party/glm/include/glm/glm.hpp"
#include "../render/labutils/vkutil.hpp" //TODO: add include folder to make includes nicer
#include "../render/labutils/to_string.hpp"
#include "../render/labutils/error.hpp"
#include <cstring>


std::vector<glm::vec3> create_regular_grid(int const& grid_resolution, std::vector<glm::vec3> const& point_cloud) {
    std::vector<glm::vec3> grid_positions;
    //Determine size of point cloud (bounding box)
    glm::vec3 min = {std::numeric_limits<float>::max(),std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    glm::vec3 max = {std::numeric_limits<float>::lowest(),std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

    for (const auto& point : point_cloud) {
        min.x = glm::min(min.x, point.x);
        min.y = glm::min(min.y, point.y);
        min.z = glm::min(min.z, point.z);

        max.x = glm::max(max.x, point.x);
        max.y = glm::max(max.y, point.y);
        max.z = glm::max(max.z, point.z);
    }

    glm::vec3 extents = glm::abs(max-min);

    float scale = 1.0f / grid_resolution;

    glm::ivec3 grid_boxes = {
            extents.x / scale,
            extents.y / scale,
            extents.z / scale,
    };



    for (unsigned int i = 0; i < grid_boxes.x ; i++) {
        for(unsigned int j = 0; j < grid_boxes.y ; j++) {
            for(unsigned int k = 0; k < grid_boxes.z ; k++) {
                grid_positions.emplace_back(
                            min.x + (i * scale),
                            min.y + (j * scale),
                            min.z + (k * scale)
                        );

            }
        }
    }

    return grid_positions;
}


//TODO: maybe for size purposes I need to round to int the scalar value to use in vertex shader.
DistanceField create_distance_field(std::vector<glm::vec3> const& grid_positions, std::vector<float> grid_scalar_value,
                                    labutils::VulkanContext const& window, labutils::Allocator const& allocator ) {

    std::uint32_t positions_size = sizeof(grid_positions[0]) * (grid_positions.size());
    std::uint32_t color_size = positions_size;

    //Add color (default to red). TODO: color will be different depending on positive / negative scalar value
    std::vector<glm::vec3> colors;
    colors.resize(grid_positions.size());
    std::fill(colors.begin(), colors.end(), glm::vec3{0.0f, 0.0f, 1.0f});

    //Add scalar value TODO: scalar value will be different depending on distance field
    grid_scalar_value.resize(grid_positions.size());
    std::fill(grid_scalar_value.begin(), grid_scalar_value.end(), 5.0f);
    std::uint32_t scalar_size = sizeof(grid_scalar_value[0]) * grid_scalar_value.size();


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

    labutils::Buffer vertexScalarGPU = labutils::create_buffer(allocator,
                                                            scalar_size,
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

    labutils::Buffer scalarStaging = labutils::create_buffer(allocator,
                                                          scalar_size,
                                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    void *posPtr = nullptr;
    if (auto const res = vmaMapMemory(allocator.allocator, posStaging.allocation, &posPtr); VK_SUCCESS !=
                                                                                            res) {
        throw labutils::Error("Mapping memory for writing\n"
                              "vmaMapMemory() returned %s", labutils::to_string(res).c_str());
    }
    std::memcpy(posPtr, grid_positions.data(), positions_size);
    vmaUnmapMemory(allocator.allocator, posStaging.allocation);

    void* colPtr = nullptr;
    if(auto const res = vmaMapMemory(allocator.allocator, colStaging.allocation, &colPtr); VK_SUCCESS != res){
        throw labutils::Error("Mapping memory for writing\n"
                              "vmaMapMemory() returned %s", labutils::to_string(res).c_str());
    }
    std::memcpy(colPtr, colors.data() , color_size);
    vmaUnmapMemory(allocator.allocator, colStaging.allocation);

    void* scalarPtr = nullptr;
    if(auto const res = vmaMapMemory(allocator.allocator, scalarStaging.allocation, &scalarPtr); VK_SUCCESS != res){
        throw labutils::Error("Mapping memory for writing\n"
                              "vmaMapMemory() returned %s", labutils::to_string(res).c_str());
    }
    std::memcpy(scalarPtr, grid_scalar_value.data() , scalar_size);
    vmaUnmapMemory(allocator.allocator, scalarStaging.allocation);

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

    VkBufferCopy scopy{};
    scopy.size = scalar_size;

    vkCmdCopyBuffer(uploadCmd, scalarStaging.buffer, vertexScalarGPU.buffer, 1, &scopy);

    labutils::buffer_barrier(uploadCmd,
                             vertexScalarGPU.buffer,
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

    return DistanceField {
            std::move(vertexPosGPU),
            std::move(vertexColGPU),
            std::move(vertexScalarGPU),
            grid_positions,
            grid_scalar_value,
            static_cast<uint32_t>(grid_positions.size())
    };







}
