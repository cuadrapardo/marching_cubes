//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#include "commands.hpp"




//Record commands for textured data
void record_commands_textured( VkCommandBuffer aCmdBuff, VkRenderPass aRenderPass, VkFramebuffer aFramebuffer,
                               VkPipeline aGraphicsPipe, VkPipeline aLinePipe, VkExtent2D const& aImageExtent,
                               VkBuffer aSceneUBO, glsl::SceneUniform const& aSceneUniform,
                               VkPipelineLayout aGraphicsLayout, VkDescriptorSet aSceneDescriptors,
                               std::vector<PointBuffer> const& points, std::vector<LineBuffer> const& lineBuffers,
                               UiConfiguration const& ui_config) {
    //Begin recording commands
    VkCommandBufferBeginInfo begInfo {};
    begInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begInfo.pInheritanceInfo = nullptr;

    if (auto const res = vkBeginCommandBuffer(aCmdBuff, &begInfo); VK_SUCCESS != res) {
        throw labutils::Error("Unable to begin recording command buffer\n"
                         "vkBeginCommandBuffer() returned %s", labutils::to_string(res).c_str());
    }
    //Upload Scene uniforms
    labutils::buffer_barrier(aCmdBuff,
                        aSceneUBO,
                        VK_ACCESS_UNIFORM_READ_BIT,
                        VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    vkCmdUpdateBuffer(aCmdBuff, aSceneUBO, 0, sizeof(glsl::SceneUniform), &aSceneUniform);

    labutils::buffer_barrier(aCmdBuff,
                        aSceneUBO,
                        VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_ACCESS_UNIFORM_READ_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
    );

    //Begin Render pass
    VkClearValue clearValues[2] {};
    clearValues[0].color.float32[0] = 0.1f; // Clear to a dark gray background.
    clearValues[0].color.float32[1] = 0.1f; // If we were debugging, this would potentially
    clearValues[0].color.float32[2] = 0.1f; // help us see whether the render pass took
    clearValues[0].color.float32[3] = 1.f; // place, even if nothing else was drawn.

    clearValues[1].depthStencil.depth = 1.f;

    VkRenderPassBeginInfo passInfo {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = aRenderPass;
    passInfo.framebuffer = aFramebuffer;
    passInfo.renderArea.offset = VkOffset2D {0, 0};
    passInfo.renderArea.extent = aImageExtent;
    passInfo.clearValueCount = 2;
    passInfo.pClearValues = clearValues;


    vkCmdBeginRenderPass(aCmdBuff, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

    //Uniforms
    vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aGraphicsLayout, 0, 1,
                            &aSceneDescriptors, 0, nullptr);

    //Begin drawing with our graphics pipeline
    vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aGraphicsPipe);

    if(ui_config.vertices) { //Draw point cloud vertices
        VkBuffer buffers[3] {points[0].positions.buffer,
                             points[0].color.buffer,
                             points[0].scale.buffer };
        VkDeviceSize offsets[3]{};

        vkCmdBindVertexBuffers(aCmdBuff, 0, 3, buffers, offsets);

        vkCmdDraw(aCmdBuff, points[0].vertex_count, 1, 0, 0);

    }

#if TEST_MODE != ON
    if(ui_config.distance_field) { //draw grid points
        VkBuffer buffers[3] {points[1].positions.buffer,
                             points[1].color.buffer,
                             points[1].scale.buffer };
        VkDeviceSize offsets[3]{};

        vkCmdBindVertexBuffers(aCmdBuff, 0, 3, buffers, offsets);

        vkCmdDraw(aCmdBuff, points[1].vertex_count, 1, 0, 0);

    }
#endif

    if(ui_config.grid) {
        //Bind line drawing pipeline
        vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aLinePipe);
#if TEST_MODE == ON
        //Note: points[1] is the grid point buffer
        VkBuffer buffers[2] {points[0].positions.buffer,
                             lineBuffers[0].color.buffer
        };
#else
        //Note: points[1] is the grid point buffer
        VkBuffer buffers[2] {points[1].positions.buffer,
                             lineBuffers[0].color.buffer
        };
#endif

        VkDeviceSize offsets[2] {};

        vkCmdBindVertexBuffers(aCmdBuff, 0, 2, buffers, offsets);

        vkCmdBindIndexBuffer(aCmdBuff, lineBuffers[0].indices.buffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw indexed
        vkCmdDrawIndexed(aCmdBuff, static_cast<uint32_t>(lineBuffers[0].vertex_count), 1, 0, 0, 0);
    }

    //End the render pass
    vkCmdEndRenderPass(aCmdBuff);

    //End command recording
    if (auto const res = vkEndCommandBuffer(aCmdBuff); VK_SUCCESS != res) {
        throw labutils::Error("unable to end recording command buffer\n"
                         "vkEndCommandBuffer() returned %s", labutils::to_string(res).c_str());
    }
}

void submit_commands(labutils::VulkanWindow const &aWindow, VkCommandBuffer aCmdBuff, VkFence aFence,
                     VkSemaphore aWaitSemaphore, VkSemaphore aSignalSemaphore) {
    VkPipelineStageFlags waitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &aCmdBuff;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &aWaitSemaphore;
    submitInfo.pWaitDstStageMask = &waitPipelineStages;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &aSignalSemaphore;

    if (auto const res = vkQueueSubmit(aWindow.graphicsQueue, 1, &submitInfo, aFence); VK_SUCCESS != res) {
        throw labutils::Error("Unable to submit command buffer to queue\n"
                         "vkQueueSubmit() returned %s", labutils::to_string(res).c_str());
    }
}

namespace ui {
    void record_commands_imgui(VkCommandBuffer cbufferImgui, labutils::VulkanWindow const& window, unsigned int const& imageIndex) {
        //Begin recording commands
        VkCommandBufferBeginInfo begInfo {};
        begInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        begInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(cbufferImgui, &begInfo); VK_SUCCESS != res) {
            throw labutils::Error("Unable to begin recording command buffer\n"
                                  "vkBeginCommandBuffer() returned %s", labutils::to_string(res).c_str());
        }

        //Transition image from present to color optimal
        labutils::image_barrier(cbufferImgui,
                                window.swapImages[imageIndex],
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VkImageSubresourceRange{
                                        VK_IMAGE_ASPECT_COLOR_BIT,
                                        0,1,
                                        0,1}
        );

        //Draw imgui into swapchain image -----------

        //Get attachment info
        //Technically just need to recalculate this when window resized
        VkRenderingAttachmentInfo colorAttachment {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.pNext = nullptr;
        colorAttachment.imageView = window.swapViews[imageIndex];
        colorAttachment.imageLayout =  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderInfo;
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.flags = 0;
        renderInfo.pNext = VK_NULL_HANDLE;
        renderInfo.renderArea = {VkOffset2D{0,0}, window.swapchainExtent} ;
        renderInfo.pColorAttachments = &colorAttachment;
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pDepthAttachment = VK_NULL_HANDLE;
        renderInfo.pStencilAttachment = VK_NULL_HANDLE;
        renderInfo.viewMask = 0;

        //Get attachment info

        vkCmdBeginRendering(cbufferImgui, &renderInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cbufferImgui);

        vkCmdEndRendering(cbufferImgui);
        // ----- Draw Imgui


        //Set swapchain image to present
        labutils::image_barrier(cbufferImgui,
                                window.swapImages[imageIndex],
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VkImageSubresourceRange{
                                        VK_IMAGE_ASPECT_COLOR_BIT,
                                        0,1,
                                        0,1}
        );

        //End command buffer
        if (auto const res = vkEndCommandBuffer(cbufferImgui); VK_SUCCESS != res) {
            throw labutils::Error("unable to end recording command buffer\n"
                                  "vkEndCommandBuffer() returned %s", labutils::to_string(res).c_str());
        }
    }

}


