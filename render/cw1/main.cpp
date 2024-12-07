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
#include <chrono>
#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>



#if !defined(GLM_FORCE_RADIANS)
#	define GLM_FORCE_RADIANS
#endif


#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

//using namespace labutils::literals;


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
#include "../labutils/to_string.hpp"
#include "../labutils/vulkan_window.hpp"
#include "../labutils/angle.hpp"
#include "../labutils/ui.hpp"

namespace lut = labutils;

#include "simple_model.hpp"
#include "load_model.hpp"
#include "output_model.hpp"
#include "point_cloud.hpp"
#include "mesh.hpp"
#include "../../marching_cubes/surface_reconstruction.hpp"
#include "../../marching_cubes/distance_field.hpp"
#include "../../marching_cubes_test/test_scene.hpp"
#include "../../incremental_remeshing/halfedge.hpp"

#if TEST_MODE == ON
    /* The test mode is designed so the user can have an interface where they can choose what vertices are positive/negative
     * This allows for testing correctness of the marching cube implementation against the original cases
     * The user modifies the test_cube_vertex_classification, where 0 is negative and 1 is positive.
     * For linear interpolation, these vertices must also have a scalar value. Since the isovalue is 1.5 (1.0f + shifted
     * by 0.5), using 1 for negative, and 2 for positive, is a simple solution. (see render_constants.hpp)
     * --> The implementation has been tested for all 22 correct base cases & produces triangles correctly. */
    std::vector<unsigned int> test_cube_vertex_classification = {
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0
    };
#endif




//Helper Vulkan functions
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



int main(int argc, char* argv[]) try
{
#pragma region Render_Setup
    // Check if the correct number of arguments is provided
#if TEST_MODE == OFF
    if (argc != 2) {
        throw std::runtime_error("Usage: program <filename>");
    }

    std::string filename = argv[1];

    // Construct the full path to the file in the "assets" folder
    std::string file_path = "assets/" + filename;
#endif

    //Create Vulkan window
    auto windowInfo = lut::make_vulkan_window();
    auto window = std::move(windowInfo.first);
//    auto limits = windowInfo.second;  // Can be used for other device features

    lut::DescriptorPool imguiDpool = lut::create_imgui_descriptor_pool(window);

    //Configure GLFW window
    UserState state{};
    glfwSetWindowUserPointer(window.window, &state);
    glfwSetKeyCallback(window.window, &glfw_callback_key_press);
    glfwSetMouseButtonCallback(window.window, &glfw_callback_button);
    glfwSetCursorPosCallback(window.window, &glfw_callback_motion);

    //Configure ImGui
    ImGui::CreateContext();
    UiConfiguration ui_config;
    ImGui_ImplGlfw_InitForVulkan(window.window, true);
    ImGui_ImplVulkan_InitInfo init_info = ui::setup_imgui(window, imguiDpool);
    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();

    //Initialise resources

    // Create VMA allocator
    lut::Allocator allocator = lut::create_allocator(window);

    lut::RenderPass renderPass = create_render_pass(window);

    lut::DescriptorSetLayout sceneLayout = create_scene_descriptor_layout(window);

    lut::PipelineLayout pipeLayout = create_pipeline_layout(window, sceneLayout.handle);
    lut::Pipeline pipe = create_pipeline(window, renderPass.handle, pipeLayout.handle);
    lut::Pipeline linePipe = create_line_pipeline(window, renderPass.handle, pipeLayout.handle);
    lut::Pipeline trianglePipe = create_triangle_pipeline(window, renderPass.handle, pipeLayout.handle);

    auto [depthBuffer, depthBufferView] = create_depth_buffer(window, allocator);

    std::vector<lut::Framebuffer> framebuffers;
    create_swapchain_framebuffers( window, renderPass.handle, framebuffers, depthBufferView.handle );

    lut::CommandPool cpool = lut::create_command_pool( window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );

    //Command buffers for standard rendering
    std::vector<VkCommandBuffer> cbuffers;
    std::vector<lut::Fence> cbfences;

    for( std::size_t i = 0; i < framebuffers.size(); ++i )
    {
        cbuffers.emplace_back( lut::alloc_command_buffer( window, cpool.handle ) );
        cbfences.emplace_back( lut::create_fence( window, VK_FENCE_CREATE_SIGNALED_BIT ) );
    }

    lut::Semaphore imageAvailable = lut::create_semaphore( window );
    lut::Semaphore renderFinished = lut::create_semaphore( window );

    //ImGui command buffers for dynamic rendering
    std::vector<VkCommandBuffer> cbuffersImgui;
    std::vector<lut::Fence> cbfencesImgui;

    for( std::size_t i = 0; i < framebuffers.size(); ++i )
    {
        cbuffersImgui.emplace_back( lut::alloc_command_buffer( window, cpool.handle ) );
        cbfencesImgui.emplace_back( lut::create_fence( window, VK_FENCE_CREATE_SIGNALED_BIT ) );
    }
    lut::Semaphore imguiImageAvailable = lut::create_semaphore(window);

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

    // Allocate descriptor set for uniform buffer
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
#pragma endregion

#if TEST_MODE == ON
    ui_config.remeshed_surface = false;
    ui_config.mc_surface = true;
    //TODO: Show points of test cube bigger
    //TODO: Show each case triangle with a different color.
    PointCloud cube = create_test_scene();
    std::vector<unsigned int> cube_edges = get_test_scene_edges();
    BoundingBox pointCloudBBox = get_bounding_box(cube.positions);

    auto [cube_edge_values, cube_edge_colors] = classify_cube_edges(cube_edges, test_cube_vertex_classification );


    //Create buffers for rendering
    PointBuffer cubeBuffer = create_pointcloud_buffers(cube.positions, cube.colors, cube.point_size,
                                                             window, allocator);
    LineBuffer lineBuffer = create_index_buffer(cube_edges, cube_edge_colors, window, allocator);

    Mesh case_triangles;
    case_triangles.positions =  query_case_table_test(test_cube_vertex_classification, cube.positions, TEST_ISOVALUE);

    case_triangles.set_normals(glm::vec3{1, 1, 0});
    case_triangles.set_color(glm::vec3{1, 0, 0});

    std::vector<PointBuffer> pBuffer;
    std::vector<LineBuffer> lBuffer;
    std::vector<MeshBuffer> mBuffer;

    if(!case_triangles.positions.empty()) { // Do not create an empty buffer - this will produce an error.
        MeshBuffer cube_triangles = create_mesh_buffer(case_triangles, window, allocator);
        mBuffer.push_back(std::move(cube_triangles));
    }

    pBuffer.push_back(std::move(cubeBuffer));
    lBuffer.push_back(std::move(lineBuffer));
#endif

#if TEST_MODE == OFF
//Load file obj, .tri, .xyz
    PointCloud pointCloud;
    pointCloud.positions = load_file(file_path, cfg::defaultConfig, ui_config);
    pointCloud.set_color(glm::vec3(0, 0.5f, 0.5f));
    pointCloud.set_size(ui_config.point_cloud_size);

    BoundingBox pointCloudBBox = get_bounding_box(pointCloud.positions);
    //TODO: set camera centre as centre of point cloud.

#pragma region "Marching Cubes"
    PointCloud distanceField; //(grid)
    std::vector<uint32_t> grid_edges; // An edge is the indices of its two vertices in the grid_positions array

    // Extend the bounding box by one cell size in each direction to avoid edge cases
    pointCloudBBox.add_padding(ui_config.padding);

    //Create grid
    auto start = std::chrono::high_resolution_clock::now();
    distanceField.positions = create_regular_grid(ui_config.grid_resolution, grid_edges, pointCloudBBox);
    distanceField.point_size = calculate_distance_field(distanceField.positions, pointCloud.positions);
    std::vector<unsigned int> vertex_classification = classify_grid_vertices(distanceField.point_size, ui_config.isovalue);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed{end-start};
    int min = static_cast<int>(elapsed.count() / 60);
    std::cout << "Time taken to create & classify distance field : " << min << "m " << elapsed.count() - (min*60) << "s " << std::endl;
    distanceField.set_color(vertex_classification);

    auto [edge_values, edge_colors] = classify_grid_edges(vertex_classification, pointCloudBBox, ui_config.grid_resolution);

    //Buffers for rendering
    std::vector<MeshBuffer> mBuffer;
    std::vector<PointBuffer> pBuffer;
    std::vector<LineBuffer> lBuffer;

    //Create marching cubes surface
    start = std::chrono::high_resolution_clock::now();
    IndexedMesh reconstructedSurfaceIndexed = query_case_table(vertex_classification, distanceField.positions, distanceField.point_size, ui_config.grid_resolution,
                                                      pointCloudBBox, ui_config.isovalue);
    end = std::chrono::high_resolution_clock::now();
    elapsed = end-start;
    min = static_cast<int>(elapsed.count() / 60);
    std::cout << "Time taken to query MC table & create mesh : " << min << "m " << elapsed.count() - (min*60) << "s " << std::endl;
    Mesh reconstructedSurface(reconstructedSurfaceIndexed);
    reconstructedSurface.set_color(glm::vec3{1.0f, 0.0f, 0.0f});
    reconstructedSurface.set_normals(glm::vec3(1.0f,1.0,0));

    //Create file and convert to HalfEdge data structure
    write_OBJ(reconstructedSurfaceIndexed, cfg::MC_obj_name);
    HalfEdgeMesh marchingCubesMesh = obj_to_halfedge(cfg::MC_obj_name);
    ui_config.mc_manifold = marchingCubesMesh.check_manifold();
    if(!ui_config.mc_manifold) {
        std::cerr << "Reconstructed Surface is not manifold (the mesh has a boundary) - increment padding " << std::endl;
        return(0);
    }

    //Calculate metrics for MC surface
    std::cout << "Calculating metrics for reconstructed surface" << std::endl;
    marchingCubesMesh.calculate_triangle_area_metrics();
    ui_config.p_cloud_to_MC_mesh = marchingCubesMesh.calculate_hausdorff_distance(pointCloud.positions);

    if(!reconstructedSurface.positions.empty()) { // Do not create an empty buffer - this will produce an error.
        MeshBuffer mc_surface = create_mesh_buffer(marchingCubesMesh, window, allocator);
        mBuffer.push_back(std::move(mc_surface));
    }
#pragma endregion

#pragma region "Remeshing"
    //Remeshing Operations
    std::cout << "Remeshing Marching Cubes surface" << std::endl;
    HalfEdgeMesh remeshedMesh = marchingCubesMesh;
    ui_config.target_edge_length = remeshedMesh.get_mean_edge_length(); //TODO: remove this?
    start = std::chrono::high_resolution_clock::now();
    remeshedMesh.remesh(ui_config.target_edge_length, ui_config.remeshing_iterations);
    end = std::chrono::high_resolution_clock::now();
    elapsed = end-start;
    min = static_cast<int>(elapsed.count() / 60);
    std::cout << "Time taken to remesh : " << min << "m " << elapsed.count() - (min*60) << "s " << std::endl;

    write_OBJ(remeshedMesh, cfg::remeshed_obj_name);

    if(!reconstructedSurface.positions.empty()) { // Do not create an empty buffer - this will produce an error.
        MeshBuffer remeshed_mesh = create_mesh_buffer(remeshedMesh, window, allocator);
        mBuffer.push_back(std::move(remeshed_mesh));
    }
    //Calculate metrics for remeshed surface
    std::cout << "Calculating metrics for remeshed surface" << std::endl;
    remeshedMesh.calculate_triangle_area_metrics();
    ui_config.MC_mesh_to_remeshed = remeshedMesh.calculate_hausdorff_distance(marchingCubesMesh.vertex_positions);
    ui_config.remesh_manifold = remeshedMesh.check_manifold();

//    return(0);


#pragma endregion

    //Create buffers for rendering
    PointBuffer pointCloudBuffer = create_pointcloud_buffers(pointCloud.positions, pointCloud.colors, pointCloud.point_size,
                                                             window, allocator);
    auto remapped_distance_field = apply_point_size_transfer_function(distanceField.point_size);
    PointBuffer gridBuffer = create_pointcloud_buffers(distanceField.positions, distanceField.colors, remapped_distance_field,
                                                       window, allocator);
    LineBuffer lineBuffer = create_index_buffer(grid_edges, edge_colors, window, allocator);

    pBuffer.push_back(std::move(pointCloudBuffer));
    pBuffer.push_back(std::move(gridBuffer));
    lBuffer.push_back(std::move(lineBuffer));

#endif

#if TEST_MODE == EDGE
    ui_config.vertices = false;
    ui_config.distance_field = false;
    ui_config.grid = false;
    ui_config.flyCamera = true;
    ui_config.mc_surface = true;

    BoundingBox pointCloudBBox;
    std::vector<PointBuffer> pBuffer;
    std::vector<LineBuffer> lBuffer;
    std::vector<MeshBuffer> mBuffer;

    HalfEdgeMesh edgeTest = obj_to_halfedge(cfg::edgeTestOBJ);

    MeshBuffer edgeTestBuffer = create_mesh_buffer(edgeTest, window, allocator);
    mBuffer.push_back(std::move(edgeTestBuffer));

    ui_config.remesh_manifold = edgeTest.check_manifold();

    ui_config.target_edge_length = edgeTest.get_mean_edge_length();

#endif
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
                linePipe = create_line_pipeline(window, renderPass.handle, pipeLayout.handle);
                trianglePipe = create_triangle_pipeline(window, renderPass.handle, pipeLayout.handle);
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
        update_user_state( state, dt, ui_config.flyCamera, (pointCloudBBox.min + pointCloudBBox.max)/2.0f);

        //Imgui Layout

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

#if TEST_MODE == EDGE

        ImGui::InputFloat("Target edge length", &ui_config.target_edge_length);

        if(ImGui::Button("Split Edges")) {
            edgeTest.split_long_edges( (4.0/3.0f) * ui_config.target_edge_length);

            vkDeviceWaitIdle(window.device);
            //Recalculate mesh buffer.
            edgeTestBuffer.vertexCount = 0;
            edgeTestBuffer = create_mesh_buffer(edgeTest, window, allocator);
            mBuffer[0] = (std::move(edgeTestBuffer));

        }

        if(ImGui::Button("Collapse Edges")) {
            edgeTest.collapse_short_edges((4.0/3.0f) * ui_config.target_edge_length, (4.0/5.0f) * ui_config.target_edge_length );

            vkDeviceWaitIdle(window.device);
            //Recalculate mesh buffer.
            edgeTestBuffer.vertexCount = 0;
            edgeTestBuffer = create_mesh_buffer(edgeTest, window, allocator);
            mBuffer[0] = (std::move(edgeTestBuffer));

        }
        if(ImGui::Button("Flip Edges")) {
            edgeTest.equalize_valences();
            vkDeviceWaitIdle(window.device);
            //Recalculate mesh buffer.
            edgeTestBuffer.vertexCount = 0;
            edgeTestBuffer = create_mesh_buffer(edgeTest, window, allocator);
            mBuffer[0] = (std::move(edgeTestBuffer));

        }

        ImGui::Text("Reconstructed surface %s manifold", ui_config.remesh_manifold ? "is" : "is not");
        if (ImGui::Button("Check manifoldness")) {
            ui_config.remesh_manifold = edgeTest.check_manifold();
        }

        if (ImGui::Button("Reset")) {
            edgeTest.reset();
            edgeTest = obj_to_halfedge(cfg::edgeTestOBJ);
            // Wait for GPU to finish processing
            vkDeviceWaitIdle(window.device);

            //Recalculate mesh buffer.
            edgeTestBuffer.vertexCount = 0;
            edgeTestBuffer = create_mesh_buffer(edgeTest, window, allocator);
            mBuffer[0] = (std::move(edgeTestBuffer));

        }

        if (ImGui::Button("Remesh")) {
            edgeTest.remesh(ui_config.target_edge_length,ui_config.remeshing_iterations);
            // Wait for GPU to finish processing
            vkDeviceWaitIdle(window.device);

            //Recalculate mesh buffer.
            edgeTestBuffer.vertexCount = 0;
            edgeTestBuffer = create_mesh_buffer(edgeTest, window, allocator);
            mBuffer[0] = (std::move(edgeTestBuffer));

        }



#endif
#if TEST_MODE == OFF
        //Camera modes not implemented ---------------------------------------------
//        ImGui::Begin("Camera Type Menu");
//        if (ImGui::RadioButton("Flying camera", ui_config.flyCamera)) {
//            ui_config.flyCamera = true;
//        }
//        ImGui::SameLine();
//        if (ImGui::RadioButton("Model camera", !ui_config.flyCamera)) {
//            ui_config.flyCamera = false;
//            state.camera2world = glm::identity<glm::mat4>(); //Reset camera
//        }
//        ImGui::End();
        //Camera modes not implemented ---------------------------------------------

        ImGui::Begin("Surface Reconstruction Menu");
        ImGui::Checkbox("View point cloud vertices", &ui_config.vertices);
//        ImGui::SliderInt("Point cloud point size",&ui_config.point_cloud_size, ui_config.p_cloud_size_min, ui_config.p_cloud_size_max); //TODO: Implement recalculation
        ImGui::Checkbox("View distance field", &ui_config.distance_field);
        ImGui::Checkbox("View 3D grid edges", &ui_config.grid);
//        ImGui::Checkbox("View vertex color", &ui_config.vertex_color); NOT IMPLEMENTED
//        ImGui::Checkbox("View edge color", &ui_config.edge_color); NOT IMPLEMENTED
        ImGui::Checkbox("View Marching Cubes surface", &ui_config.mc_surface);
        ImGui::Checkbox("View Remeshed surface", &ui_config.remeshed_surface);
        ImGui::Text("Reconstructed surface %s manifold", ui_config.mc_manifold ? "is" : "is not");
        ImGui::Text("Remeshed surface %s manifold", ui_config.remesh_manifold ? "is" : "is not");
        ImGui::SliderFloat("Grid Resolution",&ui_config.grid_resolution, ui_config.grid_resolution_min, ui_config.grid_resolution_max);
        ImGui::InputInt("Isovalue", &ui_config.isovalue);
        if (ImGui::Button("Check manifoldness")) {
            ui_config.mc_manifold = marchingCubesMesh.check_manifold();
        }
        if (ImGui::Button("Output to file")) {
            write_OBJ(reconstructedSurfaceIndexed, file_path);
        }
        if (ImGui::Button("Recalculate")) {
            // Wait for GPU to finish processing
            vkDeviceWaitIdle(window.device);

            std::cout << "Grid resolution : " << ui_config.grid_resolution << std::endl;
            //Recalculate MC surface
            reconstructedSurfaceIndexed = recalculate_grid(pointCloud, distanceField, reconstructedSurface, ui_config, pointCloudBBox,pBuffer, lBuffer, mBuffer,
                             window, allocator);
            //Recalculate remeshed surface
            remeshedMesh = recalculate_remeshed_mesh(ui_config, window, allocator, mBuffer);

        }

        ImGui::End();

        ImGui::Begin("Remeshing Menu");
        ImGui::InputFloat("Target edge length", &ui_config.target_edge_length);
        ImGui::InputInt("Remeshing iterations", &ui_config.remeshing_iterations);
        ImGui::End();

        ImGui::Begin("Marching Cubes Mesh Metrics");
        ImGui::Text("Hausdorff distance between point cloud & MC mesh: %f", ui_config.p_cloud_to_MC_mesh );
        ImGui::Text("Triangle count: %d", (int)marchingCubesMesh.faces.size()/3);
        ImGui::Text("Vertex count: %d", (int)marchingCubesMesh.vertex_positions.size());
        ImGui::Text("Average triangle area: %f", marchingCubesMesh.average_triangle_area );
        ImGui::Text("Triangle area range: %f", marchingCubesMesh.triangle_area_range );
        ImGui::Text("Triangle area standard deviation: %f", marchingCubesMesh.triangle_area_standard_deviation );
        ImGui::Text("Average triangle aspect ratio: %f", marchingCubesMesh.mean_triangle_aspect_ratio );
        ImGui::End();

        ImGui::Begin("Remeshed Mesh Metrics");
        ImGui::Text("Hausdorff distance between MC mesh & Remeshed mesh: %f", ui_config.MC_mesh_to_remeshed );
        ImGui::Text("Triangle count: %d", (int)remeshedMesh.faces.size()/3 );
        ImGui::Text("Vertex count: %d", (int)remeshedMesh.vertex_positions.size());
        ImGui::Text("Average triangle area: %f", remeshedMesh.average_triangle_area );
        ImGui::Text("Triangle area range: %f", remeshedMesh.triangle_area_range );
        ImGui::Text("Triangle area: %f", remeshedMesh.triangle_area_standard_deviation );
        ImGui::Text("Average triangle aspect ratio: %f", remeshedMesh.mean_triangle_aspect_ratio );
        ImGui::End();

#endif

#if TEST_MODE == ON
        // Create a window
        ImGui::Text("3D Cube Values");
        if (ImGui::RadioButton("Vertex 0", test_cube_vertex_classification[0] == 1)) {
            test_cube_vertex_classification[0] = test_cube_vertex_classification[0] == 1 ? 0 : 1;
        }

        if (ImGui::RadioButton("Vertex 1", test_cube_vertex_classification[1] == 1)) {
            test_cube_vertex_classification[1] = test_cube_vertex_classification[1] == 1 ? 0 : 1;
        }

        if (ImGui::RadioButton("Vertex 2", test_cube_vertex_classification[2] == 1)) {
            test_cube_vertex_classification[2] = test_cube_vertex_classification[2] == 1 ? 0 : 1;
        }

        if (ImGui::RadioButton("Vertex 3", test_cube_vertex_classification[3] == 1)) {
            test_cube_vertex_classification[3] = test_cube_vertex_classification[3] == 1 ? 0 : 1;
        }

        if (ImGui::RadioButton("Vertex 4", test_cube_vertex_classification[4] == 1)) {
            test_cube_vertex_classification[4] = test_cube_vertex_classification[4] == 1 ? 0 : 1;
        }

        if (ImGui::RadioButton("Vertex 5", test_cube_vertex_classification[5] == 1)) {
            test_cube_vertex_classification[5] = test_cube_vertex_classification[5] == 1 ? 0 : 1;
        }

        if (ImGui::RadioButton("Vertex 6", test_cube_vertex_classification[6] == 1)) {
            test_cube_vertex_classification[6] = test_cube_vertex_classification[6] == 1 ? 0 : 1;
        }

        if (ImGui::RadioButton("Vertex 7", test_cube_vertex_classification[7] == 1)) {
            test_cube_vertex_classification[7] = test_cube_vertex_classification[7] == 1 ? 0 : 1;
        }

        if (ImGui::Button("Recalculate")) {
            // Wait for GPU to finish processing
            vkDeviceWaitIdle(window.device);

            recalculate_test_scene(cube, case_triangles, cube_edges, test_cube_vertex_classification, pBuffer, lBuffer, mBuffer,window, allocator);
        }
#endif
        ImGui::Render();

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
                                          linePipe.handle,
                                          trianglePipe.handle,
                                          window.swapchainExtent,
                                          sceneUBO.buffer,
                                          sceneUniforms,
                                          pipeLayout.handle,
                                          sceneDescriptors,
                                          pBuffer,
                                          lBuffer,
                                          mBuffer,
                                          ui_config
                                          );

        submit_commands(
                window,
                cbuffers[imageIndex],
                cbfences[imageIndex].handle,
                imageAvailable.handle,
                renderFinished.handle
        );


        // Make sure that the command buffer is no longer in use
        assert( std::size_t(imageIndex) < cbfencesImgui.size() );

        if( auto const res = vkWaitForFences( window.device, 1,
                                              &cbfencesImgui[imageIndex].handle, VK_TRUE,
                                              std::numeric_limits<std::uint64_t>::max() ); VK_SUCCESS != res ){
            throw lut::Error( "Unable to wait for command buffer fence %u\n"
                              "vkWaitForFences() returned %s", imageIndex, lut::to_string(res).c_str());
        }

        if( auto const res = vkResetFences( window.device, 1, &cbfencesImgui[imageIndex].handle ); VK_SUCCESS != res ){
            throw lut::Error( "Unable to reset command buffer fence %u\n"
                              "vkResetFences() returned %s", imageIndex, lut::to_string(res).c_str());
        }
        assert(std::size_t(imageIndex) < cbuffersImgui.size());
        assert(std::size_t(imageIndex) < framebuffers.size());

       ui::record_commands_imgui(cbuffersImgui[imageIndex], window, imageIndex);

        //Submit commands
        submit_commands(
                window,
                cbuffersImgui[imageIndex],
                cbfencesImgui[imageIndex].handle,
                renderFinished.handle,
                imguiImageAvailable.handle
        );


        present_results(
                window.presentQueue,
                window.swapchain,
                imageIndex,
                imguiImageAvailable.handle,
                recreateSwapchain
        );



    }

    //TODO:  destroy imgui resources



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
