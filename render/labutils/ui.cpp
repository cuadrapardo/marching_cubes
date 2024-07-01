//
// Created by Carolina Cuadra Pardo on 6/17/24.
//
#include <iostream>
#include "ui.hpp"
#include "vkutil.hpp"
#include "to_string.hpp"
#include "error.hpp"
#include "../../third_party/glm/include/glm/glm.hpp"



namespace ui {
    /* Sets up necessary ImGui struct for Dynamic Rendering */
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

/* Deletes buffers and their allocations, recalculates grid and scalar values with given UiConfiguration
 * Populates same buffers that were deleted with updated ones
 * As this is not using a double buffer, the window will */
//TODO: Create recalculate point cloud. Maybe will be useful to resize point size- this is secondary.
void recalculate_grid(PointCloud& pointCloud, PointCloud& distanceField,
                      UiConfiguration const& ui_config, BoundingBox& bbox,
                      std::vector<PointBuffer>& pBuffer, std::vector<LineBuffer>& lineBuffer,
                      labutils::VulkanContext const& window, labutils::Allocator const& allocator) {

    // TODO: Double buffer solution for a more seamless experience
    std::cout << "Destroying buffers. The window will freeze" << std::endl;
     vmaDestroyBuffer(allocator.allocator, pBuffer[1].color.buffer, pBuffer[1].color.allocation);
     vmaDestroyBuffer(allocator.allocator, pBuffer[1].positions.buffer, pBuffer[1].positions.allocation);
     vmaDestroyBuffer(allocator.allocator, pBuffer[1].scale.buffer, pBuffer[1].scale.allocation);
     pBuffer[1].vertex_count = 0;
     vmaDestroyBuffer(allocator.allocator, lineBuffer[0].color.buffer, lineBuffer[0].color.allocation);
     vmaDestroyBuffer(allocator.allocator, lineBuffer[0].indices.buffer, lineBuffer[0].indices.allocation);
     lineBuffer[0].vertex_count = 0;

    glm::vec3 extents = glm::abs(bbox.max - bbox.min);
    float scale = 1.0f / ui_config.grid_resolution;

    glm::vec3 grid_extents = {
            extents.x / scale,
            extents.y / scale,
            extents.z / scale,
    };

    std::vector<uint32_t> grid_edges; // An edge is the indices of its two vertices in the grid_positions array
    distanceField.positions.clear();
    distanceField.colors.clear();
    distanceField.point_size.clear();
    distanceField.positions = create_regular_grid(ui_config.grid_resolution, grid_edges, bbox);
    distanceField.point_size = calculate_distance_field(distanceField.positions, pointCloud.positions);
    std::vector<unsigned int> vertex_classification = classify_grid_vertices(distanceField.point_size, ui_config.isovalue);
    distanceField.set_color(vertex_classification);

    auto [edge_values, edge_colors] = classify_grid_edges(vertex_classification, bbox, ui_config.grid_resolution);

    //Create buffers for rendering
    PointBuffer gridPointBuffer = create_pointcloud_buffers(distanceField.positions, distanceField.colors, distanceField.point_size,
                                                       window, allocator);

    LineBuffer gridLineBuffer = create_index_buffer(grid_edges, edge_colors, window, allocator);

    pBuffer[1] = (std::move(gridPointBuffer)); // Point buffer for grid points
    lineBuffer[0]  = (std::move(gridLineBuffer)); // Line buffer for grid lines
    std::cout << "Continue rendering with new buffers" << std::endl;
}
