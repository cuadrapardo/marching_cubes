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

#define DISPLAY_OVERDRAW

#include "../labutils/error.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/vkimage.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/vkbuffer.hpp"
#include "../labutils/allocator.hpp"

namespace lut = labutils;

#include "simple_model.hpp"
#include "load_model_obj.hpp"

namespace camera {
    //Camera Settings:
    constexpr float kCameraBaseSpeed = 1.7f; // units/second
    constexpr float kCameraFastMult = 5.f; // speed multiplier
    constexpr float kCameraSlowMult = 0.05f; // speed multiplier

    constexpr float kCameraMouseSensitivity = 0.01f; // radians per pixel

    enum class EInputState
    {
        forward,
        backward,
        strafeLeft,
        strafeRight,
        levitate,
        sink,
        fast,
        slow,
        mousing,
        max
    };

    struct UserState
    {
        bool inputMap[std::size_t(EInputState::max)] = {};

        float mouseX = 0.f, mouseY = 0.f;
        float previousX = 0.f, previousY = 0.f;

        bool wasMousing = false;

        glm::mat4 camera2world = glm::identity<glm::mat4>();
    };


    void update_user_state(UserState& user, float aElapsedTime);

}


struct TexturedMesh{
    labutils::Buffer positions;
    labutils::Buffer texcoords;
    labutils::Buffer color;

    std::string diffuseTexturePath;

    std::uint32_t vertexCount;
};

namespace
{
    using Clock_ = std::chrono::steady_clock;
    using Secondsf_ = std::chrono::duration<float, std::ratio<1>>;

    namespace cfg
	{
		// Compiled shader code for the graphics pipeline(s)
		// See sources in cw1/shaders/*.

#		define SHADERDIR_ "assets/cw1/shaders/"
		constexpr char const* kVertShaderPath = SHADERDIR_ "default.vert.spv";
#ifdef DISPLAY_MIPMAP_LEVELS
        constexpr char const* kFragShaderPath = SHADERDIR_ "mipmap.frag.spv";
#elif defined(DISPLAY_FRAGMENT_DEPTH)
        constexpr char const* kFragShaderPath = SHADERDIR_ "depth.frag.spv";
#elif defined(DISPLAY_PARTIAL_DERIVATIVES)
        constexpr char const* kFragShaderPath = SHADERDIR_ "partial_derivative.frag.spv";
#else
        constexpr char const* kFragShaderPath = SHADERDIR_ "default.frag.spv";
#endif
#		undef SHADERDIR_

#		define TEXTUREDIR_ "assets/cw1/textures/"
#ifdef DISPLAY_OVERSHADE
        constexpr char const* kGreenTexture = TEXTUREDIR_ "green.jpeg";
#elif defined(DISPLAY_OVERDRAW)
        constexpr char const* kGreenTexture = TEXTUREDIR_ "green.jpeg";
#endif
#		undef TEXTUREDIR_

        // Models
#       define MODELDIR_ "assets/cw1/"
        constexpr char const* sponzaObj = MODELDIR_ "sponza_with_ship.obj";
#       undef MODELDIR_


        constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;


		// General rule: with a standard 24 bit or 32 bit float depth buffer,
		// you can support a 1:1000 ratio between the near and far plane with
		// minimal depth fighting. Larger ratios will introduce more depth
		// fighting problems; smaller ratios will increase the depth buffer's
		// resolution but will also limit the view distance
		constexpr float kCameraNear  = 0.1f;
		constexpr float kCameraFar   = 100.f;

		constexpr auto kCameraFov    = 60.0_degf;
	}

	// Local functions:

    //GLFW
    void glfw_callback_key_press( GLFWwindow*, int, int, int, int );
    void glfw_callback_button(GLFWwindow*, int, int, int);
    void glfw_callback_motion(GLFWwindow* ,  double, double );

    // Uniform data
    namespace glsl
    {
        struct SceneUniform{
            glm::mat4 camera;
            glm::mat4 projection;
            glm::mat4 projCam;
        };
        //vkCmdUpdateBuffer() has certain requirements, including the two below:
        static_assert(sizeof(SceneUniform) <= 65536, "SceneUniform must be less than 65536 bytes for vkCmdUpdateBuffer");

        static_assert(sizeof(SceneUniform) % 4 == 0,
                      "SceneUniform size must be a multiple of 4 bytes");

    }

    // Helpers:
    lut::RenderPass create_render_pass( lut::VulkanWindow const& );
    std::vector<TexturedMesh> create_textured_meshes(labutils::VulkanContext const &window, labutils::Allocator const &allocator, SimpleModel& obj);

    lut::DescriptorSetLayout create_scene_descriptor_layout( lut::VulkanWindow const& );
    lut::DescriptorSetLayout create_object_descriptor_layout( lut::VulkanWindow const& );

    lut::PipelineLayout create_pipeline_layout(lut::VulkanContext const&, VkDescriptorSetLayout const&, VkDescriptorSetLayout const& aObjectLayout);
    lut::Pipeline create_pipeline(lut::VulkanWindow const &aWindow, VkRenderPass aRenderPass, VkPipelineLayout aPipelineLayout);

    std::tuple<lut::Image, lut::ImageView> create_depth_buffer(lut::VulkanWindow const&, lut::Allocator const&);


    void create_swapchain_framebuffers(
            lut::VulkanWindow const&,
            VkRenderPass,
            std::vector<lut::Framebuffer>&,
            VkImageView aDepthView
    );

    void update_scene_uniforms(
            glsl::SceneUniform&,
            std::uint32_t aFramebufferWidth,
            std::uint32_t aFramebufferHeight,
            camera::UserState const& state
    );


    void record_commands_textured( VkCommandBuffer,
                                   VkRenderPass,
                                   VkFramebuffer,
                                   VkPipeline,
                                   VkExtent2D const&,
                                   VkBuffer aSceneUBO,
                                   glsl::SceneUniform const&,
                                   VkPipelineLayout,
                                   VkDescriptorSet sceneDescriptors,
                                   std::vector<TexturedMesh> const& meshes,
                                   std::vector<VkDescriptorSet> const& meshDescriptors);

    void submit_commands(
            lut::VulkanWindow const&,
            VkCommandBuffer,
            VkFence,
            VkSemaphore,
            VkSemaphore
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
    camera::UserState state{};
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
#ifdef DISPLAY_OVERSHADE
            //Use a green texture for all meshes.
            lut::Image currentTex =  lut::load_image_texture2d(cfg::kGreenTexture, window, loadCmdPool.handle, allocator);
#elif defined(DISPLAY_OVERDRAW)
            //Use a green texture for all meshes.
            lut::Image currentTex =  lut::load_image_texture2d(cfg::kGreenTexture, window, loadCmdPool.handle, allocator);
#else
            lut::Image currentTex = lut::load_image_texture2d(texturedMeshes[mesh].diffuseTexturePath.c_str(), window, loadCmdPool.handle, allocator);
#endif
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

#ifdef DISPLAY_MIPMAP_LEVELS
    std::cout << "Running program without anisotropic filtering." << std::endl;
    anisotropicFiltering = false; // Anisotropic filtering should be disabled when visualising mipmaps.
#endif

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

//region helpers

namespace
{
    void glfw_callback_key_press( GLFWwindow* aWindow, int aKey, int /*aScanCode*/, int aAction, int /*aModifierFlags*/ )
    {
        auto state = static_cast<camera::UserState*>(glfwGetWindowUserPointer( aWindow ));
        assert( state );

        bool const isReleased = (GLFW_RELEASE == aAction);

        switch( aKey ) {
            case GLFW_KEY_W:
                state->inputMap[std::size_t(camera::EInputState::forward)] = !isReleased;
                break;
            case GLFW_KEY_S:
                state->inputMap[std::size_t(camera::EInputState::backward)] = !isReleased;
                break;
            case GLFW_KEY_A:
                state->inputMap[std::size_t(camera::EInputState::strafeLeft)] = !isReleased;
                break;
            case GLFW_KEY_D:
                state->inputMap[std::size_t(camera::EInputState::strafeRight)] = !isReleased;
                break;
            case GLFW_KEY_E:
                state->inputMap[std::size_t(camera::EInputState::levitate)] = !isReleased;
                break;
            case GLFW_KEY_Q:
                state->inputMap[std::size_t(camera::EInputState::sink)] = !isReleased;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                [[fallthrough]];
            case GLFW_KEY_RIGHT_SHIFT:
                state->inputMap[std::size_t(camera::EInputState::fast)] = !isReleased;
                break;
            case GLFW_KEY_LEFT_CONTROL:
                [[fallthrough]];
            case GLFW_KEY_RIGHT_CONTROL:
                state->inputMap[std::size_t(camera::EInputState::slow)] = !isReleased;
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose( aWindow, GLFW_TRUE );
                break;
            default:
                ;
        }
    }

    void glfw_callback_button( GLFWwindow* aWin, int aBut, int aAct, int ){
        auto state = static_cast<camera::UserState*>(glfwGetWindowUserPointer( aWin ));
        assert( state );

        if( GLFW_MOUSE_BUTTON_RIGHT == aBut && GLFW_PRESS == aAct ){
            auto& flag = state->inputMap[std::size_t(camera::EInputState::mousing)];

            flag = !flag;
            if( flag )
                glfwSetInputMode( aWin, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
            else
                glfwSetInputMode( aWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
        }
    }

    void glfw_callback_motion( GLFWwindow* aWin, double aX, double aY ) {
        auto state = static_cast<camera::UserState*>(glfwGetWindowUserPointer( aWin ));
        assert( state );
        state->mouseX = float(aX);
        state->mouseY = float(aY);
    }

}

namespace camera {
    void update_user_state( UserState& aState, float aElapsedTime ){
        auto& cam = aState.camera2world;

        if( aState.inputMap[std::size_t(EInputState::mousing)] )
        {
            // Only update the rotation on the second frame of mouse navigation. This ensures that the previousX
            // and Y variables are initialized to sensible values.
            if( aState.wasMousing )
            {
                auto const sens = camera::kCameraMouseSensitivity;
                auto const dx = sens * (aState.mouseX-aState.previousX);
                auto const dy = sens * (aState.mouseY-aState.previousY);

                cam = cam * glm::rotate( -dy, glm::vec3( 1.f, 0.f, 0.f ) );
                cam = cam * glm::rotate( -dx, glm::vec3( 0.f, 1.f, 0.f ) );
            }

            aState.previousX = aState.mouseX;
            aState.previousY = aState.mouseY;
            aState.wasMousing = true;
        }
        else
        {
            aState.wasMousing = false;
        }

        auto const move = aElapsedTime * camera::kCameraBaseSpeed *
                          (aState.inputMap[std::size_t(EInputState::fast)] ? camera::kCameraFastMult:1.f) *
                          (aState.inputMap[std::size_t(EInputState::slow)] ? camera::kCameraSlowMult:1.f);
        if( aState.inputMap[std::size_t(EInputState::forward)] )
            cam = cam * glm::translate( glm::vec3( 0.f, 0.f, -move ) );
        if( aState.inputMap[std::size_t(EInputState::backward)] )
            cam = cam * glm::translate( glm::vec3( 0.f, 0.f, +move ) );

        if( aState.inputMap[std::size_t(EInputState::strafeLeft)] )
            cam = cam * glm::translate( glm::vec3( -move, 0.f, 0.f ) );
        if( aState.inputMap[std::size_t(EInputState::strafeRight)] )
            cam = cam * glm::translate( glm::vec3( +move, 0.f, 0.f ) );

        if( aState.inputMap[std::size_t(EInputState::levitate)] )
            cam = cam * glm::translate( glm::vec3( 0.f, +move, 0.f ) );
        if( aState.inputMap[std::size_t(EInputState::sink)] )
            cam = cam * glm::translate( glm::vec3( 0.f, -move, 0.f ) );
    }
}

namespace {
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

            lut::Buffer vertexPosGPU = lut::create_buffer(
                    allocator,
                    positionsSize,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    0, //no additional VmaAllocationCreateFlags
                    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE //or just VMA_MEMORY_USAGE_AUTO
            );
            lut::Buffer vertexTexGPU = lut::create_buffer(allocator,
                                                          texcoordsSize,
                                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                          0,
                                                          VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
            );
            lut::Buffer vertexColGPU = lut::create_buffer(allocator,
                                                          colorSize,
                                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                          0,
                                                          VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
            );
            lut::Buffer posStaging = lut::create_buffer(allocator,
                                                        positionsSize,
                                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
            lut::Buffer texStaging = lut::create_buffer(allocator,
                                                        texcoordsSize,
                                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
            lut::Buffer colStaging = lut::create_buffer(allocator,
                                                        colorSize,
                                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

            void *posPtr = nullptr;
            if (auto const res = vmaMapMemory(allocator.allocator, posStaging.allocation, &posPtr); VK_SUCCESS !=
                                                                                                    res) {
                throw lut::Error("Mapping memory for writing\n"
                                 "vmaMapMemory() returned %s", lut::to_string(res).c_str());
            }
            std::memcpy(posPtr, meshPositions.data(), positionsSize);
            vmaUnmapMemory(allocator.allocator, posStaging.allocation);

            void *texPtr = nullptr;
            if (auto const res = vmaMapMemory(allocator.allocator, texStaging.allocation, &texPtr); VK_SUCCESS !=
                                                                                                    res) {
                throw lut::Error("Mapping memory for writing\n"
                                 "vmaMapMemory() returned %s", lut::to_string(res).c_str());
            }
            std::memcpy(texPtr, meshTexcoords.data(), texcoordsSize);
            vmaUnmapMemory(allocator.allocator, texStaging.allocation);

            void* colPtr = nullptr;
            if(auto const res = vmaMapMemory(allocator.allocator, colStaging.allocation, &colPtr); VK_SUCCESS != res){
                throw lut::Error("Mapping memory for writing\n"
                                 "vmaMapMemory() returned %s", lut::to_string(res).c_str());
            }
            std::memcpy(colPtr, meshColor.data() , colorSize);
            vmaUnmapMemory(allocator.allocator, colStaging.allocation);

            //We need to ensure that the Vulkan resources are alive until all the transfers have completed. For simplicity,
            //we will just wait for the operations to complete with a fence. A more complex solution might want to queue
            //transfers, let these take place in the background while performing other tasks.
            lut::Fence uploadComplete = create_fence(window);

            //Queue data uploads from staging buffers to the final buffers.
            //This uses a separate command pool for simplicity.
            lut::CommandPool uploadPool = create_command_pool(window);
            VkCommandBuffer uploadCmd = alloc_command_buffer(window, uploadPool.handle);

            //Record the copy commands into the command buffer
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            if (auto const res = vkBeginCommandBuffer(uploadCmd, &beginInfo); VK_SUCCESS != res) {
                throw lut::Error("Beginning command buffer recording\n"
                                 "vkBeginCommandBuffer() returned %s", lut::to_string(res).c_str());
            }

            VkBufferCopy pcopy{};
            pcopy.size = positionsSize;

            vkCmdCopyBuffer(uploadCmd, posStaging.buffer, vertexPosGPU.buffer, 1, &pcopy);

            lut::buffer_barrier(uploadCmd,
                                vertexPosGPU.buffer,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

            VkBufferCopy tcopy{};
            tcopy.size = texcoordsSize;

            vkCmdCopyBuffer(uploadCmd, texStaging.buffer, vertexTexGPU.buffer, 1, &tcopy);

            lut::buffer_barrier(uploadCmd,
                                vertexTexGPU.buffer,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
            );

            VkBufferCopy ccopy{};
            ccopy.size = colorSize;

            vkCmdCopyBuffer(uploadCmd, colStaging.buffer, vertexColGPU.buffer, 1, &ccopy);

            lut::buffer_barrier(uploadCmd,
                                vertexColGPU.buffer,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
            );
            if(auto const res = vkEndCommandBuffer(uploadCmd);VK_SUCCESS != res) {
                throw lut::Error("Ending command buffer recording\n"
                                 "vkEndCommandBUffer() returned %s", lut::to_string(res).c_str());
            }

            //Submit transfer commands
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &uploadCmd;

            if (auto const res = vkQueueSubmit(window.graphicsQueue, 1, &submitInfo, uploadComplete.handle);
                    VK_SUCCESS !=
                    res) {
                throw lut::Error("Submitting commands\n"
                                 "vkQueueSubmit() returned %s", lut::to_string(res).c_str());
            }

            //Wait for commands to finish before we destroy the temporary resources required for the transfers
            // ( staging buffers, command pool,...)
            //
            //The code doesn't destroy the resources implicitly. The resources are destroyed by the destructors of the labutils
            //wrappers for the various objects once we leave the function's scope.

            if (auto const res = vkWaitForFences(window.device, 1, &uploadComplete.handle, VK_TRUE,
                                                 std::numeric_limits<std::uint64_t>::max());
                    VK_SUCCESS != res) {
                throw lut::Error("Waiting for upload to complete\n"
                                 "vkWaitForFences() returned %s", lut::to_string(res).c_str());
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
    void update_scene_uniforms( glsl::SceneUniform& aSceneUniforms, std::uint32_t aFramebufferWidth, std::uint32_t aFramebufferHeight, camera::UserState const& aState)
    {
        float const aspect = aFramebufferWidth / float(aFramebufferHeight);

        aSceneUniforms.projection = glm::perspectiveRH_ZO(
                lut::Radians(cfg::kCameraFov).value(),
                aspect,
                cfg::kCameraNear,
                cfg::kCameraFar
        );
        aSceneUniforms.projection[1][1] *= -1.f; //Mirror y axis
        aSceneUniforms.camera = glm::translate(glm::vec3(0.f, -0.3f, -1.f));
        aSceneUniforms.camera = aSceneUniforms.camera * glm::inverse(aState.camera2world);

        aSceneUniforms.projCam = aSceneUniforms.projection * aSceneUniforms.camera;

    }
}

namespace {
    lut::RenderPass create_render_pass(lut::VulkanWindow const &aWindow) {
        VkAttachmentDescription attachments[2]{};
        attachments[0].format = aWindow.swapchainFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        attachments[1].format = cfg::kDepthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference subpassAttachments[1]{};
        subpassAttachments[0].attachment = 0; //this refers to attachments[0]
        subpassAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachment{};
        depthAttachment.attachment = 1;
        depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpasses[1]{};
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].colorAttachmentCount = 1;
        subpasses[0].pColorAttachments = subpassAttachments;
        subpasses[0].pDepthStencilAttachment = &depthAttachment;

        //Requires a subpass dependency to ensure that the first transition happens after the presentation engine is done
        //with it.

        VkSubpassDependency deps[2]{};
        deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].srcAccessMask = 0;
        deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[0].dstSubpass = 0;
        deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        deps[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        deps[1].dstSubpass = 0;
        deps[1].dstAccessMask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        VkRenderPassCreateInfo passInfo{};
        passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        passInfo.attachmentCount = 2;
        passInfo.pAttachments = attachments;
        passInfo.subpassCount = 1;
        passInfo.pSubpasses = subpasses;
        passInfo.dependencyCount = 2;
        passInfo.pDependencies = deps;

        VkRenderPass rpass = VK_NULL_HANDLE;
        if (auto const res = vkCreateRenderPass(aWindow.device, &passInfo, nullptr, &rpass); VK_SUCCESS != res) {
            throw lut::Error("Unable to create render pass\n"
                             "vkCreateRenderPass() returned %s", lut::to_string(res).c_str());
        }
        return lut::RenderPass(aWindow.device, rpass);
    }

    lut::PipelineLayout create_pipeline_layout(lut::VulkanContext const &aContext, VkDescriptorSetLayout const &aSceneLayout, VkDescriptorSetLayout const& aObjectLayout ) {
        VkDescriptorSetLayout layouts[] = {
                //Order must match the set=N in the shaders
                aSceneLayout, //Set 0
                aObjectLayout //set 1
        };

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
        layoutInfo.pSetLayouts = layouts;
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;

        VkPipelineLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreatePipelineLayout(aContext.device, &layoutInfo, nullptr, &layout); VK_SUCCESS !=
                                                                                                     res) {
            throw lut::Error("Unable to create pipeline layout\n"
                             "vkCreatePipelineLayout() returned %s", lut::to_string(res).c_str());
        }
        return lut::PipelineLayout(aContext.device, layout);
    }


    lut::Pipeline create_pipeline(lut::VulkanWindow const &aWindow, VkRenderPass aRenderPass, VkPipelineLayout aPipelineLayout) {

        //Load shader modules
        lut::ShaderModule vert = lut::load_shader_module(aWindow, cfg::kVertShaderPath);
        lut::ShaderModule frag = lut::load_shader_module(aWindow, cfg::kFragShaderPath);

        //Define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        //Vertex shader
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vert.handle;
        stages[0].pName = "main";

        //Enable depth testing
        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

#ifdef DISPLAY_OVERDRAW
        //We want to disable early z discard to visualise overdraw.
        depthInfo.depthTestEnable = VK_FALSE;
        depthInfo.depthWriteEnable = VK_FALSE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        depthInfo.depthBoundsTestEnable = VK_FALSE;
        depthInfo.stencilTestEnable = VK_FALSE;

#else
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;
#endif

        //Fragment shader
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = frag.handle;
        stages[1].pName = "main";

        //Position
        VkVertexInputBindingDescription vertexInputs[3]{};
        vertexInputs[0].binding = 0;
        vertexInputs[0].stride = sizeof(float) * 3;
        vertexInputs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        //Texcoords
        vertexInputs[1].binding = 1;
        vertexInputs[1].stride = sizeof(float) * 2;
        vertexInputs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        //Color
        vertexInputs[2].binding = 2;
        vertexInputs[2].stride = sizeof(float) * 3;
        vertexInputs[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vertexAttributes[3]{};
        vertexAttributes[0].binding = 0; //must match binding above
        vertexAttributes[0].location = 0; //must match shader
        vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[0].offset = 0;

        vertexAttributes[1].binding = 1; // must match binding above
        vertexAttributes[1].location = 1; //ditto
        vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributes[1].offset = 0;

        vertexAttributes[2].binding = 2; //must match binding above
        vertexAttributes[2].location = 2; //must match shader
        vertexAttributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[2].offset = 0;

        //Vertex input state
        VkPipelineVertexInputStateCreateInfo inputInfo{};
        inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        inputInfo.vertexBindingDescriptionCount = 3; //number of vertexInputs above
        inputInfo.pVertexBindingDescriptions = vertexInputs;
        inputInfo.vertexAttributeDescriptionCount = 3; //number of vertexAttributes above
        inputInfo.pVertexAttributeDescriptions = vertexAttributes;

        //Input Assembly State
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        assemblyInfo.primitiveRestartEnable = VK_FALSE;

        //Viewport state. Define viewport and scissor regions
        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = float(aWindow.swapchainExtent.width);
        viewport.height = float(aWindow.swapchainExtent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.offset = VkOffset2D{0, 0};
        scissor.extent = VkExtent2D{aWindow.swapchainExtent.width, aWindow.swapchainExtent.height};

        VkPipelineViewportStateCreateInfo viewportInfo{};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &scissor;

        //Rasterization state- define rasterisation options
        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.lineWidth = 1.f; // required.

        //Define multisampling state
        VkPipelineMultisampleStateCreateInfo samplingInfo{};
        samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        //Define blend state
        VkPipelineColorBlendAttachmentState blendStates[1] = {};
        VkPipelineColorBlendStateCreateInfo blendInfo = {};

#ifdef DISPLAY_OVERSHADE
        // Set up blend attachment state for additive blending to visualise overdraw

        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendStates[0].blendEnable = VK_TRUE;
        blendStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendStates[0].colorBlendOp = VK_BLEND_OP_ADD;
        blendStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendStates[0].alphaBlendOp = VK_BLEND_OP_ADD;

        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = &blendStates[0];
#elif defined(DISPLAY_OVERDRAW)
        //We need to include fragments which would get normally discarded by early-z tests.
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendStates[0].blendEnable = VK_TRUE;
        blendStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendStates[0].colorBlendOp = VK_BLEND_OP_ADD;
        blendStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendStates[0].alphaBlendOp = VK_BLEND_OP_ADD;

        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = &blendStates[0];
#else
    // We define one blend state per colour attachment.
        blendStates[0].blendEnable = VK_FALSE;
        blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                        VK_COLOR_COMPONENT_A_BIT;

        blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendInfo.logicOpEnable = VK_FALSE;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = blendStates;

#endif

        //Create Pipeline
        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        pipeInfo.stageCount = 2; //vertex + fragment
        pipeInfo.pStages = stages;
        pipeInfo.pVertexInputState = &inputInfo;
        pipeInfo.pInputAssemblyState = &assemblyInfo;
        pipeInfo.pTessellationState = nullptr; // no tessellation
        pipeInfo.pViewportState = &viewportInfo;
        pipeInfo.pRasterizationState = &rasterInfo;
        pipeInfo.pMultisampleState = &samplingInfo;
        pipeInfo.pDepthStencilState = &depthInfo;
        pipeInfo.pColorBlendState = &blendInfo;
        pipeInfo.pDynamicState = nullptr; // no dynamic states

        pipeInfo.layout = aPipelineLayout;
        pipeInfo.renderPass = aRenderPass;
        pipeInfo.subpass = 0; //first subpass of aRenderPass

        VkPipeline pipe = VK_NULL_HANDLE;
        if (auto const res = vkCreateGraphicsPipelines(aWindow.device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipe);
                VK_SUCCESS != res) {
            throw lut::Error("Unable to create graphics pipeline\n"
                             "vkCreateGraphicsPipelines() returned %s", lut::to_string(res).c_str());
        }

        return lut::Pipeline(aWindow.device, pipe);
    }

    void create_swapchain_framebuffers(lut::VulkanWindow const &aWindow, VkRenderPass aRenderPass,
                                       std::vector<lut::Framebuffer> &aFramebuffers, VkImageView aDepthView) {
        assert(aFramebuffers.empty());

        for (std::size_t i = 0; i < aWindow.swapViews.size(); ++i) {
            VkImageView attachments[2] = {
                    aWindow.swapViews[i],
                    aDepthView
            };
            VkFramebufferCreateInfo fbInfo{};
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

    lut::DescriptorSetLayout create_scene_descriptor_layout(lut::VulkanWindow const &aWindow) {
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
            throw lut::Error("Unable to create descriptor set layoutt\n"
                             "vkCreateDescriptorSetLAyout() returned %s", lut::to_string(res).c_str());
        }
        return lut::DescriptorSetLayout(aWindow.device, layout);
    }

    lut::DescriptorSetLayout create_object_descriptor_layout(lut::VulkanWindow const &aWindow) {
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
            throw lut::Error("Unable to create descriptor set layout\n"
                             "vkCreateDescriptorSetLayout() returned %s", lut::to_string(res).c_str());
        }
        return lut::DescriptorSetLayout(aWindow.device, layout);

    }

    std::tuple<lut::Image, lut::ImageView>
    create_depth_buffer(lut::VulkanWindow const &aWindow, lut::Allocator const &aAllocator) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = cfg::kDepthFormat;
        imageInfo.extent.width = aWindow.swapchainExtent.width;
        imageInfo.extent.height = aWindow.swapchainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        if (auto const res = vmaCreateImage(aAllocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr);
                VK_SUCCESS != res) {
            throw lut::Error("Unable to allocate depth buffer image.\n"
                             "vmaCreateImage() returned %s", lut::to_string(res).c_str()
            );
        }

        lut::Image depthImage(aAllocator.allocator, image, allocation);

        // Create the image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = cfg::kDepthFormat;
        viewInfo.components = VkComponentMapping{};
        viewInfo.subresourceRange = VkImageSubresourceRange{
                VK_IMAGE_ASPECT_DEPTH_BIT,
                0, 1,
                0, 1
        };

        VkImageView view = VK_NULL_HANDLE;
        if (auto const res = vkCreateImageView(aWindow.device, &viewInfo, nullptr, &view);VK_SUCCESS != res) {
            throw lut::Error("Unable to create image view\n"
                             "vkCreateImageView() returned %s", lut::to_string(res).c_str()
            );
        }

        return {std::move(depthImage), lut::ImageView(aWindow.device, view)};
    }

    //Record commands for textured data
    void record_commands_textured( VkCommandBuffer aCmdBuff, VkRenderPass aRenderPass, VkFramebuffer aFramebuffer,
                                   VkPipeline aGraphicsPipe, VkExtent2D const& aImageExtent, VkBuffer aSceneUBO,
                                   glsl::SceneUniform const& aSceneUniform,  VkPipelineLayout aGraphicsLayout,
                                   VkDescriptorSet aSceneDescriptors, std::vector<TexturedMesh> const& meshes, std::vector<VkDescriptorSet> const& meshDescriptors){
        //Begin recording commands
        VkCommandBufferBeginInfo begInfo{};
        begInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        begInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(aCmdBuff, &begInfo); VK_SUCCESS != res) {
            throw lut::Error("Unable to begin recording command buffer\n"
                             "vkBeginCommandBuffer() returned %s", lut::to_string(res).c_str());
        }
        //Upload Scene uniforms
        lut::buffer_barrier(aCmdBuff,
                            aSceneUBO,
                            VK_ACCESS_UNIFORM_READ_BIT,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        vkCmdUpdateBuffer(aCmdBuff, aSceneUBO, 0, sizeof(glsl::SceneUniform), &aSceneUniform);

        lut::buffer_barrier(aCmdBuff,
                            aSceneUBO,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_UNIFORM_READ_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );

        //Begin Render pass
        VkClearValue clearValues[2]{};
        clearValues[0].color.float32[0] = 0.1f; // Clear to a dark gray background.
        clearValues[0].color.float32[1] = 0.1f; // If we were debugging, this would potentially
        clearValues[0].color.float32[2] = 0.1f; // help us see whether the render pass took
        clearValues[0].color.float32[3] = 1.f; // place, even if nothing else was drawn.

        clearValues[1].depthStencil.depth = 1.f;

        VkRenderPassBeginInfo passInfo{};
        passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        passInfo.renderPass = aRenderPass;
        passInfo.framebuffer = aFramebuffer;
        passInfo.renderArea.offset = VkOffset2D{0, 0};
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
        for(unsigned int mesh = 0; mesh < meshes.size(); mesh++){
            //Bind vertex input
            VkBuffer buffers[3] = {meshes[mesh].positions.buffer, meshes[mesh].texcoords.buffer, meshes[mesh].color.buffer};
            VkDeviceSize offsets[3]{};


            //Bind texture
            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aGraphicsLayout, 1, 1, &meshDescriptors[mesh],
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
            throw lut::Error("unable to end recording command buffer\n"
                             "vkEndCommandBuffer() returned %s", lut::to_string(res).c_str());
        }


    }

    void submit_commands(lut::VulkanWindow const &aWindow, VkCommandBuffer aCmdBuff, VkFence aFence,
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
            throw lut::Error("Unable to submit command buffer to queue\n"
                             "vkQueueSubmit() returned %s", lut::to_string(res).c_str());
        }
    }

    void present_results(VkQueue aPresentQueue, VkSwapchainKHR aSwapchain, std::uint32_t aImageIndex,
                         VkSemaphore aRenderFinished, bool &aNeedToRecreateSwapchain) {
        //Present rendered images.
        VkPresentInfoKHR presentInfo{};
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
