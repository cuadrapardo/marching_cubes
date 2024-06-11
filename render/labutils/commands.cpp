//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#include "commands.hpp"


//Record commands for textured data
void record_commands_textured( VkCommandBuffer aCmdBuff, VkRenderPass aRenderPass, VkFramebuffer aFramebuffer,
                               VkPipeline aGraphicsPipe, VkExtent2D const& aImageExtent, VkBuffer aSceneUBO,
                               glsl::SceneUniform const& aSceneUniform,  VkPipelineLayout aGraphicsLayout,
                               VkDescriptorSet aSceneDescriptors, std::vector<TexturedMesh> const& meshes, std::vector<VkDescriptorSet> const& meshDescriptors) {
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

    //Begin drawing with our graphics pipeline
    vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aGraphicsPipe);

    //Uniforms
    vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aGraphicsLayout, 0, 1,
                            &aSceneDescriptors, 0, nullptr);

    //Bind and draw all meshes in the scene
    for (unsigned int mesh = 0; mesh < meshes.size(); mesh++) {
        //Bind vertex input
        VkBuffer buffers[3] = {meshes[mesh].positions.buffer, meshes[mesh].texcoords.buffer, meshes[mesh].color.buffer};
        VkDeviceSize offsets[3] {};


        //Bind texture
        vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aGraphicsLayout, 1, 1,
                                &meshDescriptors[mesh],
                                0,
                                nullptr);

        vkCmdBindVertexBuffers(aCmdBuff, 0, 3, buffers, offsets);

        //Draw vertices
        vkCmdDraw(aCmdBuff, meshes[mesh].vertexCount, 1, 0, 0);

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


