//
// Created by Carolina Cuadra Pardo on 6/17/24.
//
#include <iostream>
#include "ui.hpp"
#include "vkutil.hpp"
#include "to_string.hpp"
#include "error.hpp"



namespace ui {
    ImGui_ImplVulkan_InitInfo
    setup_imgui(labutils::VulkanWindow const& window, labutils::DescriptorPool const& imguiDpool) {
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = window.instance;
        init_info.PhysicalDevice = window.physicalDevice;
        init_info.Device = window.device;
        init_info.Queue = window.graphicsQueue;
        init_info.DescriptorPool = imguiDpool.handle;
        init_info.MinImageCount = window.swapImages.size();
        init_info.ImageCount = window.swapImages.size();
        init_info.UseDynamicRendering = true;


        //dynamic rendering parameters for imgui to use
        init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &window.swapchainFormat;

        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        return init_info;
    }



}

//!!! DOES NOT WORK:

void recalculate_grid(PointCloud& pointCloud, PointCloud& distanceField,
                      UiConfiguration const& ui_config,
                      std::vector<PointBuffer>& pBuffer, std::vector<LineBuffer>& lineBuffer,
                      labutils::VulkanContext const& window, labutils::Allocator const& allocator) {

    // IMPORTANT calculate everything in bg while scene is drawn. THEN wait idle. Then use new vertex data. This should make it more seamless

    std::cout << "Destroying buffers. The window will freeze" << std::endl;


     vmaDestroyBuffer(allocator.allocator, pBuffer[1].color.buffer, pBuffer[1].color.allocation);
     vmaDestroyBuffer(allocator.allocator, pBuffer[1].positions.buffer, pBuffer[1].positions.allocation);
     vmaDestroyBuffer(allocator.allocator, pBuffer[1].scale.buffer, pBuffer[1].scale.allocation);
     pBuffer[1].vertex_count = 0;
     vmaDestroyBuffer(allocator.allocator, lineBuffer[0].color.buffer, lineBuffer[0].color.allocation);
     vmaDestroyBuffer(allocator.allocator, lineBuffer[0].indices.buffer, lineBuffer[0].indices.allocation);
     lineBuffer[0].vertex_count = 0;

//    pointCloud.set_color(glm::vec3(1.0f, 0, 0));
//    pointCloud.set_size(point_size); ONLY RECALCULATING GRID. TODO: move this elsewhere
    std::vector<uint32_t> grid_edges; // An edge is the indices of its two vertices in the grid_positions array
    std::vector<glm::vec3> edge_colors;

    distanceField.positions.clear();
    distanceField.colors.clear();
    distanceField.point_size.clear();
    distanceField.positions = create_regular_grid(ui_config.grid_resolution, pointCloud.positions, grid_edges);
    distanceField.point_size = calculate_distance_field(distanceField.positions, pointCloud.positions);
    std::vector<bool> vertex_classification = classify_grid_vertices(distanceField.point_size, ui_config.isovalue);
    distanceField.set_color(vertex_classification);

    //TODO: change edge color depending on bipolar
    edge_colors.resize(grid_edges.size());
    std::fill(edge_colors.begin(), edge_colors.end(), glm::vec3{0.0f, 1.0f, 0.0});



    //Create buffers for rendering
//    PointBuffer pointCloudBuffer = create_pointcloud_buffers(pointCloud.positions, pointCloud.colors, pointCloud.point_size,
//                                                             window, allocator);
    PointBuffer gridPointBuffer = create_pointcloud_buffers(distanceField.positions, distanceField.colors, distanceField.point_size,
                                                       window, allocator);

    LineBuffer gridLineBuffer = create_index_buffer(grid_edges, edge_colors, window, allocator);


//    pBuffer.push_back(&pointCloudBuffer);
    pBuffer[1] = (std::move(gridPointBuffer));
    lineBuffer[0]  = (std::move(gridLineBuffer));

    std::cout << "Continue rendering with new buffers" << std::endl;
}
