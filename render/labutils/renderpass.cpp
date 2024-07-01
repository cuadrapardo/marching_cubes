//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#include "renderpass.hpp"

labutils::RenderPass create_render_pass(labutils::VulkanWindow const &aWindow) {
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
        throw labutils::Error("Unable to create render pass\n"
                         "vkCreateRenderPass() returned %s", labutils::to_string(res).c_str());
    }
    return labutils::RenderPass(aWindow.device, rpass);
}

labutils::RenderPass create_imgui_render_pass(labutils::VulkanWindow const& aWindow) {
    VkAttachmentDescription attachment = {};
    attachment.format = aWindow.swapchainFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    VkRenderPass imguiPass = VK_NULL_HANDLE;

    if (auto const res = vkCreateRenderPass(aWindow.device, &info, nullptr, &imguiPass); VK_SUCCESS != res) {
        throw labutils::Error("Unable to create render pass\n"
                              "vkCreateRenderPass() returned %s", labutils::to_string(res).c_str());
    }
    return labutils::RenderPass(aWindow.device, imguiPass);

}

std::tuple<labutils::Image, labutils::ImageView> create_depth_buffer(labutils::VulkanWindow const& aWindow, labutils::Allocator const& aAllocator) {
    VkImageCreateInfo imageInfo {};
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

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    if (auto const res = vmaCreateImage(aAllocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr);
            VK_SUCCESS != res) {
        throw labutils::Error("Unable to allocate depth buffer image.\n"
                         "vmaCreateImage() returned %s", labutils::to_string(res).c_str()
        );
    }

    labutils::Image depthImage(aAllocator.allocator, image, allocation);

    // Create the image view
    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = cfg::kDepthFormat;
    viewInfo.components = VkComponentMapping {};
    viewInfo.subresourceRange = VkImageSubresourceRange {
            VK_IMAGE_ASPECT_DEPTH_BIT,
            0, 1,
            0, 1
    };

    VkImageView view = VK_NULL_HANDLE;
    if (auto const res = vkCreateImageView(aWindow.device, &viewInfo, nullptr, &view);VK_SUCCESS != res) {
        throw labutils::Error("Unable to create image view\n"
                         "vkCreateImageView() returned %s", labutils::to_string(res).c_str()
        );
    }

    return {std::move(depthImage), labutils::ImageView(aWindow.device, view)};
}

labutils::PipelineLayout create_pipeline_layout(labutils::VulkanContext const &aContext, VkDescriptorSetLayout const &aSceneLayout ) {
    VkDescriptorSetLayout layouts[] = {
            //Order must match the set=N in the shaders
            aSceneLayout //Set 0
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
        throw labutils::Error("Unable to create pipeline layout\n"
                         "vkCreatePipelineLayout() returned %s", labutils::to_string(res).c_str());
    }
    return labutils::PipelineLayout(aContext.device, layout);
}

labutils::Pipeline create_line_pipeline(labutils::VulkanWindow const &aWindow, VkRenderPass aRenderPass, VkPipelineLayout aPipelineLayout) {
    //Load shader modules
    labutils::ShaderModule vert = labutils::load_shader_module(aWindow, cfg::kLineVertShaderPath);
    labutils::ShaderModule frag = labutils::load_shader_module(aWindow, cfg::kFragShaderPath);

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

    depthInfo.depthTestEnable = VK_TRUE;
    depthInfo.depthWriteEnable = VK_TRUE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthInfo.minDepthBounds = 0.f;
    depthInfo.maxDepthBounds = 1.f;

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

    //Color
    vertexInputs[1].binding = 1;
    vertexInputs[1].stride = sizeof(float) * 3;
    vertexInputs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;



    VkVertexInputAttributeDescription vertexAttributes[3]{};
    vertexAttributes[0].binding = 0; //must match binding above
    vertexAttributes[0].location = 0; //must match shader
    vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[0].offset = 0;

    vertexAttributes[1].binding = 1; // must match binding above
    vertexAttributes[1].location = 1; //ditto
    vertexAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[1].offset = 0;


    //Vertex input state
    VkPipelineVertexInputStateCreateInfo inputInfo{};
    inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputInfo.vertexBindingDescriptionCount = 2; //number of vertexInputs above
    inputInfo.pVertexBindingDescriptions = vertexInputs;
    inputInfo.vertexAttributeDescriptionCount = 2; //number of vertexAttributes above
    inputInfo.pVertexAttributeDescriptions = vertexAttributes;

    //Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
    assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; //render lines
    assemblyInfo.primitiveRestartEnable = VK_FALSE; //Potential issue?

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

    // We define one blend state per colour attachment.
    blendStates[0].blendEnable = VK_FALSE;
    blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT;

    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = blendStates;

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
        throw labutils::Error("Unable to create graphics pipeline\n"
                              "vkCreateGraphicsPipelines() returned %s", labutils::to_string(res).c_str());
    }

    return labutils::Pipeline(aWindow.device, pipe);

}

labutils::Pipeline create_triangle_pipeline(labutils::VulkanWindow const &aWindow, VkRenderPass aRenderPass, VkPipelineLayout aPipelineLayout) {
    //Load shader modules
    labutils::ShaderModule vert = labutils::load_shader_module(aWindow, cfg::kTriVertShaderPath);
    labutils::ShaderModule frag = labutils::load_shader_module(aWindow, cfg::kTriFragShaderPath);

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

    depthInfo.depthTestEnable = VK_TRUE;
    depthInfo.depthWriteEnable = VK_TRUE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthInfo.minDepthBounds = 0.f;
    depthInfo.maxDepthBounds = 1.f;

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

    //Color
    vertexInputs[1].binding = 1;
    vertexInputs[1].stride = sizeof(float) * 3;
    vertexInputs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    //Normals
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
    vertexAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
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
#if TEST_MODE == ON
    rasterInfo.cullMode = VK_CULL_MODE_NONE;
    rasterInfo.polygonMode = VK_POLYGON_MODE_LINE;
#else
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
     rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
#endif
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

    // We define one blend state per colour attachment.
    blendStates[0].blendEnable = VK_FALSE;
    blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT;

    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = blendStates;

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
        throw labutils::Error("Unable to create graphics pipeline\n"
                              "vkCreateGraphicsPipelines() returned %s", labutils::to_string(res).c_str());
    }

    return labutils::Pipeline(aWindow.device, pipe);
}

labutils::Pipeline create_pipeline(labutils::VulkanWindow const &aWindow, VkRenderPass aRenderPass, VkPipelineLayout aPipelineLayout) {

    //Load shader modules
    labutils::ShaderModule vert = labutils::load_shader_module(aWindow, cfg::kVertShaderPath);
    labutils::ShaderModule frag = labutils::load_shader_module(aWindow, cfg::kFragShaderPath);

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

    depthInfo.depthTestEnable = VK_TRUE;
    depthInfo.depthWriteEnable = VK_TRUE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthInfo.minDepthBounds = 0.f;
    depthInfo.maxDepthBounds = 1.f;

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

    //Color
    vertexInputs[1].binding = 1;
    vertexInputs[1].stride = sizeof(float) * 3;
    vertexInputs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    //Scalar value
    vertexInputs[2].binding = 2;
    vertexInputs[2].stride = sizeof(int);
    vertexInputs[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


    VkVertexInputAttributeDescription vertexAttributes[3]{};
    vertexAttributes[0].binding = 0; //must match binding above
    vertexAttributes[0].location = 0; //must match shader
    vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[0].offset = 0;

    vertexAttributes[1].binding = 1; // must match binding above
    vertexAttributes[1].location = 1; //ditto
    vertexAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[1].offset = 0;

    vertexAttributes[2].binding = 2; // must match binding above
    vertexAttributes[2].location = 2; //ditto
    vertexAttributes[2].format = VK_FORMAT_R32_SINT; //Technically does not need to be signed
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
    assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; //render points
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

    // We define one blend state per colour attachment.
    blendStates[0].blendEnable = VK_FALSE;
    blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT;

    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = blendStates;

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
        throw labutils::Error("Unable to create graphics pipeline\n"
                         "vkCreateGraphicsPipelines() returned %s", labutils::to_string(res).c_str());
    }

    return labutils::Pipeline(aWindow.device, pipe);
}
