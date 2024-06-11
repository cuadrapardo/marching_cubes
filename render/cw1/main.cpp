#include <volk/volk.h>

#include <tuple>
#include <chrono>
#include <limits>
#include <vector>
#include <stdexcept>

#include <cstdio>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if !defined(GLM_FORCE_RADIANS)
#	define GLM_FORCE_RADIANS
#endif
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <iostream>

#include "../labutils/to_string.hpp"
#include "../labutils/vulkan_window.hpp"

#include "../labutils/angle.hpp"
using namespace labutils::literals;


#include "../labutils/error.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/vkimage.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/camera.hpp"
#include "../labutils/vkbuffer.hpp"
#include "../labutils/input.hpp"
#include "../labutils/renderpass.hpp"
#include "../labutils/allocator.hpp"
#include "../labutils/render_constants.hpp"
#include "../labutils/descriptor.hpp"
#include "../labutils/commands.hpp"

namespace lut = labutils;

#include "simple_model.hpp"
#include "load_model_obj.hpp"


namespace
{
    using Clock_ = std::chrono::steady_clock;
    using Secondsf_ = std::chrono::duration<float, std::ratio<1>>;

    void create_swapchain_framebuffers(
            lut::VulkanWindow const&,
            VkRenderPass,
            std::vector<lut::Framebuffer>&,
            VkImageView aDepthView
    );

    void present_results(
            VkQueue,
            VkSwapchainKHR,
            std::uint32_t aImageIndex,
            VkSemaphore,
            bool& aNeedToRecreateSwapchain
    );

}


int main() try
{
    //Create Vulkan window
    auto windowInfo = lut::make_vulkan_window();
    auto window = std::move(windowInfo.first);
    auto limits = windowInfo.second;

    std::cout<<" Max bias: " << limits.maxSamplerLodBias << std::endl;

    //Configure GLFW window
    UserState state{};
    glfwSetWindowUserPointer(window.window, &state);
    glfwSetKeyCallback(window.window, &glfw_callback_key_press);
    glfwSetMouseButtonCallback(window.window, &glfw_callback_button);
    glfwSetCursorPosCallback(window.window, &glfw_callback_motion);


    // Create VMA allocator
    lut::Allocator allocator = lut::create_allocator(window);

    //Initialise resources
    lut::RenderPass renderPass = create_render_pass(window);

    lut::DescriptorSetLayout sceneLayout = create_scene_descriptor_layout(window);
    lut::DescriptorSetLayout objectLayout = create_object_descriptor_layout(window);

    lut::PipelineLayout pipeLayout = create_pipeline_layout(window, sceneLayout.handle, objectLayout.handle);
    lut::Pipeline pipe = create_pipeline(window, renderPass.handle, pipeLayout.handle);

    auto [depthBuffer, depthBufferView] = create_depth_buffer(window, allocator);

    std::vector<lut::Framebuffer> framebuffers;
    create_swapchain_framebuffers( window, renderPass.handle, framebuffers, depthBufferView.handle );

    lut::CommandPool cpool = lut::create_command_pool( window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );

    std::vector<VkCommandBuffer> cbuffers;
    std::vector<lut::Fence> cbfences;

    for( std::size_t i = 0; i < framebuffers.size(); ++i )
    {
        cbuffers.emplace_back( lut::alloc_command_buffer( window, cpool.handle ) );
        cbfences.emplace_back( lut::create_fence( window, VK_FENCE_CREATE_SIGNALED_BIT ) );
    }

    lut::Semaphore imageAvailable = lut::create_semaphore( window );
    lut::Semaphore renderFinished = lut::create_semaphore( window );


//Important: keep the image & image views alive during main loop

    //Create scene uniform buffer
    lut::Buffer sceneUBO = lut::create_buffer(
            allocator,
            sizeof(glsl::SceneUniform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    );

    //Create descriptor pool
    lut::DescriptorPool dPool = lut::create_descriptor_pool(window);

    // allocate descriptor set for uniform buffer
    VkDescriptorSet sceneDescriptors = lut::alloc_desc_set(window, dPool.handle, sceneLayout.handle);
    {
        VkWriteDescriptorSet desc[1]{};

        VkDescriptorBufferInfo sceneUboInfo{};
        sceneUboInfo.buffer = sceneUBO.buffer,
                sceneUboInfo.range = VK_WHOLE_SIZE;

        desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[0].dstSet = sceneDescriptors;
        desc[0].dstBinding = 0;
        desc[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc[0].descriptorCount = 1;
        desc[0].pBufferInfo = &sceneUboInfo;

        constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
        vkUpdateDescriptorSets(window.device, numSets, desc, 0, nullptr);
    }

    // Load obj file
    SimpleModel obj = load_simple_wavefront_obj(cfg::sponzaObj);

    //Create textured meshes from the obj
    std::vector<TexturedMesh> texturedMeshes = create_textured_meshes(window, allocator, obj);

    //Load textures into image
    std::vector<lut::Image>  meshImages;
    {
        lut::CommandPool loadCmdPool = lut::create_command_pool(window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
        for(unsigned int mesh = 0; mesh < texturedMeshes.size(); ++mesh) {
            lut::Image currentTex = lut::load_image_texture2d(texturedMeshes[mesh].diffuseTexturePath.c_str(), window, loadCmdPool.handle, allocator);
            meshImages.emplace_back(std::move(currentTex));
        }
    }


        std::vector<lut::ImageView> meshImageViews;
       //Create image views for textured images
       for (unsigned int img = 0; img < meshImages.size(); ++img){
           lut::ImageView aView = lut::create_image_view_texture2d(window, meshImages[img].image, VK_FORMAT_R8G8B8A8_SRGB);
            meshImageViews.emplace_back(std::move(aView));

       }


    //Create default texture sampler
    bool anisotropicFiltering = lut::anisotropic_filtering(window.physicalDevice);
    anisotropicFiltering = false; // Anisotropic filtering should be disabled when visualising mipmaps.

       lut::Sampler defaultSampler = lut::create_default_sampler(window, anisotropicFiltering, limits.maxSamplerAnisotropy );


       std::vector<VkDescriptorSet> descriptors;

       //allocate and initialise descriptor sets for textures
       for(unsigned int tex = 0; tex < meshImageViews.size(); ++tex){
           VkDescriptorSet meshDescriptors = lut::alloc_desc_set( window, dPool.handle,  objectLayout.handle );
           {
               VkWriteDescriptorSet desc[1]{};

               VkDescriptorImageInfo textureInfo{};
               textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
               textureInfo.imageView = meshImageViews[tex].handle;
               textureInfo.sampler = defaultSampler.handle;

               desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
               desc[0].dstSet = meshDescriptors;
               desc[0].dstBinding = 0;
               desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
               desc[0].descriptorCount = 1;
               desc[0].pImageInfo = &textureInfo;


               constexpr auto numSets = sizeof(desc)/sizeof(desc[0]);
               vkUpdateDescriptorSets( window.device, numSets, desc, 0, nullptr );
           }
           descriptors.emplace_back(meshDescriptors);
       }


       // It is better to not create a descriptor set per texture, as meshes will reuse the same textures.
       // Instead, check if the texture with that path has been created and if so, just use that.



    auto previousClock = Clock_::now();

    // Application main loop
    bool recreateSwapchain = false;

    while( !glfwWindowShouldClose( window.window ) )
    {
        // Let GLFW process events.
        // glfwPollEvents() checks for events, processes them. If there are no
        // events, it will return immediately. Alternatively, glfwWaitEvents()
        // will wait for any event to occur, process it, and only return at
        // that point. The former is useful for applications where you want to
        // render as fast as possible, whereas the latter is useful for
        // input-driven applications, where redrawing is only needed in
        // reaction to user input (or similar).
        glfwPollEvents(); // or: glfwWaitEvents()


        // Recreate swap chain
        if( recreateSwapchain )
        {
            //We need to destroy several objects, which may still be in use by the GPU. Therefore, first wait for the
            //GPU to finish processing
            vkDeviceWaitIdle(window.device);

            //Recreate them
            auto const changes = recreate_swapchain(window);

            if(changes.changedFormat){
                renderPass = create_render_pass(window);
            }

            if(changes.changedSize){
                std::tie(depthBuffer, depthBufferView) = create_depth_buffer(window, allocator);
                pipe = create_pipeline(window, renderPass.handle, pipeLayout.handle);
            }

            framebuffers.clear();

            create_swapchain_framebuffers(window, renderPass.handle, framebuffers, depthBufferView.handle);



            recreateSwapchain = false;
            continue;
        }


        //Acquire next swap chain image
        std::uint32_t imageIndex = 0;
        auto const acquireRes = vkAcquireNextImageKHR(
                window.device,
                window.swapchain,
                std::numeric_limits<std::uint64_t>::max(),
                imageAvailable.handle,
                VK_NULL_HANDLE,
                &imageIndex
        );
        if(VK_SUBOPTIMAL_KHR == acquireRes || VK_ERROR_OUT_OF_DATE_KHR == acquireRes){
            // This occurs e.g., when the window has been resized. In this case we need to recreate the swap chain to
            // match the new dimensions. Any resources that directly depend on the swap chain need to be recreated
            // as well. While rare, re-creating the swap chain may give us a different image format, which we should
            // handle.
            //
            // In both cases, we set the flag that the swap chain has to be re-created and jump to the top of the loop.
            // Technically, with the VK SUBOPTIMAL KHR return code, we could continue rendering with the
            // current swap chain (unlike VK ERROR OUT OF DATE KHR, which does require us to recreate the
            // swap chain).
            recreateSwapchain = true;
            continue;
        }

        if(VK_SUCCESS != acquireRes){
            throw lut::Error("unable to acquire next swapchain image\n"
                             "vkAcquireNextImageKHR() returned %s", lut::to_string(acquireRes).c_str());
        }

        //Update state
        auto const now = Clock_::now();
        auto const dt = std::chrono::duration_cast<Secondsf_>(now-previousClock).count();
        previousClock = now;
        update_user_state( state, dt );

        //Prepare data for this frame
        glsl::SceneUniform sceneUniforms{};
        update_scene_uniforms(sceneUniforms, window.swapchainExtent.width, window.swapchainExtent.height, state );

        // Make sure that the command buffer is no longer in use
        assert( std::size_t(imageIndex) < cbfences.size() );

        if( auto const res = vkWaitForFences( window.device, 1,
                                              &cbfences[imageIndex].handle, VK_TRUE,
                                              std::numeric_limits<std::uint64_t>::max() ); VK_SUCCESS != res ){
            throw lut::Error( "Unable to wait for command buffer fence %u\n"
                              "vkWaitForFences() returned %s", imageIndex, lut::to_string(res).c_str());
        }

        if( auto const res = vkResetFences( window.device, 1, &cbfences[imageIndex].handle ); VK_SUCCESS != res ){
            throw lut::Error( "Unable to reset command buffer fence %u\n"
                              "vkResetFences() returned %s", imageIndex, lut::to_string(res).c_str());
        }
        //record and submit commands
        assert(std::size_t(imageIndex) < cbuffers.size());
        assert(std::size_t(imageIndex) < framebuffers.size());

        // record and submit commands

        record_commands_textured(cbuffers[imageIndex],
                                          renderPass.handle,
                                          framebuffers[imageIndex].handle,
                                          pipe.handle,
                                          window.swapchainExtent,
                                          sceneUBO.buffer,
                                          sceneUniforms,
                                          pipeLayout.handle,
                                          sceneDescriptors,
                                          texturedMeshes,
                                          descriptors);

        submit_commands(
                window,
                cbuffers[imageIndex],
                cbfences[imageIndex].handle,
                imageAvailable.handle,
                renderFinished.handle
        );

        present_results(
                window.presentQueue,
                window.swapchain,
                imageIndex,
                renderFinished.handle,
                recreateSwapchain
        );

    }

    // Cleanup takes place automatically in the destructors, but we sill need
    // to ensure that all Vulkan commands have finished before that.
    vkDeviceWaitIdle( window.device );

	return 0;
}
catch( std::exception const& eErr )
{
	std::fprintf( stderr, "\n" );
	std::fprintf( stderr, "Error: %s\n", eErr.what() );
	return 1;
}



namespace {

    void create_swapchain_framebuffers(lut::VulkanWindow const& aWindow, VkRenderPass aRenderPass,
                                       std::vector<lut::Framebuffer>& aFramebuffers, VkImageView aDepthView) {
        assert(aFramebuffers.empty());

        for (std::size_t i = 0; i < aWindow.swapViews.size(); ++i) {
            VkImageView attachments[2] = {
                    aWindow.swapViews[i],
                    aDepthView
            };
            VkFramebufferCreateInfo fbInfo {};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.flags = 0;
            fbInfo.renderPass = aRenderPass;
            fbInfo.attachmentCount = 2;
            fbInfo.pAttachments = attachments;
            fbInfo.width = aWindow.swapchainExtent.width;
            fbInfo.height = aWindow.swapchainExtent.height;
            fbInfo.layers = 1;

            VkFramebuffer fb = VK_NULL_HANDLE;
            if (auto const res = vkCreateFramebuffer(aWindow.device, &fbInfo, nullptr, &fb); VK_SUCCESS != res) {
                throw lut::Error("Unable to create framebuffer for swapchain image %zu\n"
                                 "vkCreateFrameBuffer() returned %s", i, lut::to_string(res).c_str());
            }
            aFramebuffers.emplace_back(lut::Framebuffer(aWindow.device, fb));
        }

        assert(aWindow.swapViews.size() == aFramebuffers.size());
    }


    void present_results(VkQueue aPresentQueue, VkSwapchainKHR aSwapchain, std::uint32_t aImageIndex,
                         VkSemaphore aRenderFinished, bool& aNeedToRecreateSwapchain) {
        //Present rendered images.
        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &aRenderFinished;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &aSwapchain;
        presentInfo.pImageIndices = &aImageIndex;
        presentInfo.pResults = nullptr;

        auto const presentRes = vkQueuePresentKHR(aPresentQueue, &presentInfo);

        if (VK_SUBOPTIMAL_KHR == presentRes || VK_ERROR_OUT_OF_DATE_KHR == presentRes) {
            aNeedToRecreateSwapchain = true;
        } else if (VK_SUCCESS != presentRes) {
            throw lut::Error("Unable present swapchain image %u\n"
                             "vkQueuePresentKHR() returned %s", aImageIndex, lut::to_string(presentRes).c_str());
        }
    }
}


//endregion


//EOF vim:syntax=cpp:foldmethod=marker:ts=4:noexpandtab: 
