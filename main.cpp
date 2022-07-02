#include <array>
#include <string.h>
#include <memory>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "Defines.hpp"
#include "vkmEnums.h"
#include "vkmInit.hpp"
#include "Helpers.hpp"
#include "Resources.hpp"
#include "Loader.hpp"

// #define MESH_SHADING

struct AppManager
{
    GLFWwindow *window;
    uint32_t windowWidth = 500;
    uint32_t windowHeight = 500;

    glm::mat4 projMatrix { 1.0f };

    bool initDone = false;
    bool displayGui = true;

    bool mouseDown = false;
    glm::vec2 prevMousePos { 0.0f, 0.0f };

    uint32_t indexCount[BUFFER_COUNT];

} g_app;

struct ConfigManager
{
    glm::vec3 dirLightIntensity { 0.0f, 0.0f, 0.0f };
    glm::vec3 dirLightPosition { 0.0f, 0.0f, 0.0f };

    glm::vec3 materialAlbedo { 1.0f, 0.0f, 0.0f };
    float materialRoughness { 0.5f };
} g_config;

struct Camera {
    double yaw = -1.0f * glm::half_pi<double>();
    double pitch = 0.0f;
    glm::mat4 matrix { 1.0f };
    glm::vec3 pos { 0.0f, 0.0f, 0.0f };
    bool dirty = false;
} g_camera;

VulkanResources g_vk;

struct PerFrameUBO
{
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec4 viewPos; // making vec4 for now... worry about alignment later
};

struct PerMatUBO
{
    glm::vec4 albedo; // making vec4 for now... worry about alignment later
    glm::vec4 roughness;
};

struct LightUBO
{
    glm::vec4 position; // making vec4 for now... worry about alignment later
    glm::vec4 intensity; // making vec4 for now... worry about alignment later
};

// -------------------------
// GLFW INPUT HANDLER
// -------------------------

static void update_camera() {

    glm::vec3 pos {
        glm::cos(g_camera.yaw) * glm::cos(g_camera.pitch),
        glm::sin(g_camera.pitch),
        glm::sin(g_camera.yaw) * glm::cos(g_camera.pitch) };

    pos *= 2;
    
    glm::vec3 forward = glm::normalize(-1.0f * pos);
    glm::vec3 up { 0.0f, 1.0f, 0.0f };
    glm::vec3 right = glm::normalize(glm::cross(forward, up));
    up = glm::normalize(glm::cross(right, forward));

    g_camera.matrix = glm::lookAt(pos, forward, up);
    g_camera.pos = pos;
    g_camera.dirty = true;

    // This wasn't giving correct reutls, idk why
    // g_camera.matrix = {
    //     right.x, right.y, right.z, -g_camera.pos.x,
    //     up.x, up.y, up.z, -g_camera.pos.y,
    //     forward.x, forward.y, forward.z, -g_camera.pos.z, 
    //     0.0f, 0.0f, 0.0f, 1.0f
    // };
}


static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    if (g_app.mouseDown) {
        // drag across screen = pi radians of rotation
        const double delta_x = -1.0f * glm::pi<double>() * (g_app.prevMousePos.x - xpos) / g_app.windowWidth ;
        const double delta_y = 1.0f * glm::pi<double>() * (g_app.prevMousePos.y - ypos) / g_app.windowHeight;

        g_camera.yaw += delta_x;
        if (g_camera.yaw > glm::two_pi<double>()) g_camera.yaw -= glm::two_pi<double>();
        if (g_camera.yaw < -1.0f * glm::two_pi<double>()) g_camera.yaw += glm::two_pi<double>();

        g_camera.pitch = glm::clamp(g_camera.pitch + delta_y, -1.0f * glm::half_pi<double>() + 0.01f, glm::half_pi<double>() - 0.01f);

        update_camera();
    }

    g_app.prevMousePos.x = xpos;
    g_app.prevMousePos.y = ypos;
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!g_app.initDone)
        return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        auto io = ImGui::GetIO();
        if (!io.WantCaptureMouse)
            g_app.mouseDown = true;
        else
            g_app.mouseDown = false;
    }
    else {
        g_app.mouseDown = false;
    }
}

// -------------------------
// RENDERPASS
// -------------------------

void createRenderPass()
{
    const std::array<VkAttachmentDescription, 2> attachments{{
        {
            // Color
            .format = g_vk.swapchain.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        {
            // Depth
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        },
    }};

    const std::array<VkAttachmentReference, 1> colorAttachmentReferences {{
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        }
    }};

    const VkAttachmentReference depthAttachmentReference {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    const VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReferences.size()),
        .pColorAttachments = colorAttachmentReferences.data(),
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = &depthAttachmentReference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr};

    const VkSubpassDependency dependencies[2]{
        {// First dependency at the start of the renderpass
            // Does the transition from final to initial layout
            .srcSubpass = VK_SUBPASS_EXTERNAL,                             // Producer of the dependency
            .dstSubpass = 0,                                               // Consumer is our single subpass that will wait for the execution dependency
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Match our pWaitDstStageMask when we vkQueueSubmit
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // is a loadOp stage for color attachments
            .srcAccessMask = 0,                                            // semaphore wait already does memory dependency for us
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,         // is a loadOp CLEAR access mask for color attachments
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT},
        {// Second dependency at the end the renderpass
            // Does the transition from the initial to the final layout
            // Technically this is the same as the implicit subpass dependency, but we are gonna state it explicitly here
            .srcSubpass = 0,                                               // Producer of the dependency is our single subpass
            .dstSubpass = VK_SUBPASS_EXTERNAL,                             // Consumer are all commands outside of the renderpass
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // is a storeOp stage for color attachments
            .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // Do not block any subsequent work
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,         // is a storeOp `STORE` access mask for color attachments
            .dstAccessMask = 0,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT}};

    const VkRenderPassCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 2,
        .pDependencies = dependencies};

    VK_CHECK(vkCreateRenderPass(g_vk.device, &createInfo, nullptr, &g_vk.renderPass));
}

void createFramebuffers()
{
    createAttachment(g_vk.device, VK_FORMAT_D32_SFLOAT, { g_vk.swapchain.extent.width, g_vk.swapchain.extent.height, 1 }, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, g_vk.attachments[ATTACHMENT_DEPTH]);

    VkFramebufferCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = g_vk.renderPass,
        .attachmentCount = 2,
        .width = g_vk.swapchain.extent.width,
        .height = g_vk.swapchain.extent.height,
        .layers = 1};

    g_vk.framebuffers.resize(g_vk.swapchain.imageViews.size());
    for (size_t i = 0; i < g_vk.framebuffers.size(); ++i)
    {
        VkImageView attachments[2] = {
            g_vk.swapchain.imageViews[i],
            g_vk.attachments[ATTACHMENT_DEPTH].view};

        createInfo.pAttachments = attachments;

        VK_CHECK(vkCreateFramebuffer(g_vk.device, &createInfo, nullptr, &g_vk.framebuffers[i]));
    }
}


// -------------------------
// DESCRIPTORS
// -------------------------

void createDesriptorPools()
{
{
    // No idea how many descriptors imgui needs
    std::array<VkDescriptorPoolSize, 11> poolSizes{{{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}}};

    const VkDescriptorPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .maxSets = 1u,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(g_vk.device, &createInfo, nullptr, &g_vk.descriptorPools[DESCRIPTOR_POOL_IMGUI]));
}
{
    std::array<VkDescriptorPoolSize, 1> poolSizes{{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
    }};

    const VkDescriptorPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .maxSets = 2u,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(g_vk.device, &createInfo, nullptr, &g_vk.descriptorPools[DESCRIPTOR_POOL_DEFAULT]));
}
}

void createDescriptorSetLayouts()
{
    std::array<VkDescriptorSetLayoutBinding, 2> set0Bindings{{
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
    }};

    VkDescriptorSetLayoutCreateInfo set0LayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .bindingCount = static_cast<uint32_t>(set0Bindings.size()),
        .pBindings = set0Bindings.data(),
    };

    vkCreateDescriptorSetLayout(g_vk.device, &set0LayoutCreateInfo, nullptr, &g_vk.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_DEFAULT_0]);

    std::array<VkDescriptorSetLayoutBinding, 1> set1Bindings{{
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        }
    }};

    VkDescriptorSetLayoutCreateInfo set1LayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .bindingCount = static_cast<uint32_t>(set1Bindings.size()),
        .pBindings = set1Bindings.data(),
    };

    vkCreateDescriptorSetLayout(g_vk.device, &set1LayoutCreateInfo, nullptr, &g_vk.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_DEFAULT_1]);
}

void createDescriptorSets()
{
    VkDescriptorSetAllocateInfo frameSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = g_vk.descriptorPools[DESCRIPTOR_POOL_DEFAULT],
        .descriptorSetCount = 1,
        .pSetLayouts = &g_vk.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_DEFAULT_0],
    };

    vkAllocateDescriptorSets(g_vk.device, &frameSetAllocInfo, &g_vk.descriptorSets[DESCRIPTOR_SET_FRAME]);

    VkDescriptorSetAllocateInfo materialSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = g_vk.descriptorPools[DESCRIPTOR_POOL_DEFAULT],
        .descriptorSetCount = 1,
        .pSetLayouts = &g_vk.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_DEFAULT_1],
    };

    vkAllocateDescriptorSets(g_vk.device, &materialSetAllocInfo, &g_vk.descriptorSets[DESCRIPTOR_SET_MATERIAL]);
}

void updateDescriptorSets()
{
{   // Frame
    const std::array<VkDescriptorBufferInfo, 2> descriptorBufferInfo {{
        {
            .buffer = g_vk.buffers[BUFFER_PER_FRAME_UBO].buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        },
        {
            .buffer = g_vk.buffers[BUFFER_LIGHT_UBO].buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        },
    }};

    const std::array<VkWriteDescriptorSet, 2> writes {{
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = g_vk.descriptorSets[DESCRIPTOR_SET_FRAME],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &descriptorBufferInfo[0],
            .pTexelBufferView = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = g_vk.descriptorSets[DESCRIPTOR_SET_FRAME],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &descriptorBufferInfo[1],
            .pTexelBufferView = nullptr,
        },
    }};

    vkUpdateDescriptorSets(g_vk.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

{   // Material
    const VkDescriptorBufferInfo descriptorBufferInfo {
        .buffer = g_vk.buffers[BUFFER_MATERIAL_UBO].buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };

    const std::array<VkWriteDescriptorSet, 1> writes {{
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = g_vk.descriptorSets[DESCRIPTOR_SET_MATERIAL],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &descriptorBufferInfo,
            .pTexelBufferView = nullptr,
        },
    }};

    vkUpdateDescriptorSets(g_vk.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
}


// -------------------------
// PIPELINES
// -------------------------

void createPipelineLayouts()
{
    std::array<VkDescriptorSetLayout, 2> setLayouts{
        g_vk.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_DEFAULT_0],
        g_vk.descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_DEFAULT_1]};

    // std::array<VkPushConstantRange, 1> ranges {{
    //     {
    //         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    //         .offset = 0,
    //         .size = sizeof(PushConst),
    //     }
    // }};

    const VkPipelineLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
        // .setLayoutCount = 0,
        // .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0, // static_cast<uint32_t>(ranges.size()),
        .pPushConstantRanges = nullptr // ranges.data(),
    };

    VK_CHECK(vkCreatePipelineLayout(g_vk.device, &createInfo, nullptr, &g_vk.pipelineLayout));
}

void createPipelines()
{
    const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageCreateInfo{{{
                                                                                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                                                        .stage = VK_SHADER_STAGE_VERTEX_BIT,
                                                                                        .module = createShaderModule(g_vk.device, "../shaders/spirv/default-vert.spv"),
                                                                                        .pName = "main",
                                                                                    },
                                                                                    {
                                                                                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                                                        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                                        .module = createShaderModule(g_vk.device, "../shaders/spirv/default-frag.spv"),
                                                                                        .pName = "main",
                                                                                    }}};
    
    const std::array<VkVertexInputBindingDescription, 1> vertexInputBindings {{
        {
            .binding = 0,
            .stride = 32,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        }
    }};

    const std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributes {{
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0,
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 12,
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 20,
        },
    }};

    const VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size()),
        .pVertexBindingDescriptions = vertexInputBindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size()),
        .pVertexAttributeDescriptions = vertexInputAttributes.data()
    };

    const VkViewport viewport{
        .x = 0,
        .y = 0,
        .width = static_cast<float>(g_vk.swapchain.extent.width),
        .height = static_cast<float>(g_vk.swapchain.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    const VkRect2D scissorRect{
        .offset = {.x = 0, .y = 0},
        .extent = g_vk.swapchain.extent};

    const VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissorRect,
    };

    const VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    const VkPipelineColorBlendAttachmentState colorBlendAttachmentState{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
        .blendConstants = {0, 0, 0, 0}};

    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    const VkGraphicsPipelineCreateInfo pipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStageCreateInfo.size()),
        .pStages = shaderStageCreateInfo.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .layout = g_vk.pipelineLayout,
        .renderPass = g_vk.renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    VK_CHECK(vkCreateGraphicsPipelines(g_vk.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &g_vk.pipeline));

    vkDestroyShaderModule(g_vk.device, shaderStageCreateInfo[0].module, nullptr);
    vkDestroyShaderModule(g_vk.device, shaderStageCreateInfo[1].module, nullptr);
}


// -------------------------
// COMMAND POOLS / BUFFERS
// -------------------------

void createCommandPools()
{
    g_vk.commandPools[COMMAND_POOL_DEFAULT] = createCommandPool(g_vk.device, g_vk.queueFamilyIndices[QUEUE_GRAPHICS]);
}

void createCommandBuffers()
{
    g_vk.commandBuffers[COMMAND_BUFFER_DEFAULT] = createCommandBuffer(g_vk.device, g_vk.commandPools[COMMAND_POOL_DEFAULT]);
}

// -------------------------
// SYNCHRONIZATION RESOURCES
// -------------------------

void createSynchornizationResources()
{
    g_vk.fences[FENCE_IMAGE_ACQUIRE] = createFence(g_vk.device, false);
}


// -------------------------
// APP 
// -------------------------

void init()
{
    const vkmInitParams initParams {
        .window = g_app.window,
        .windowWidth = g_app.windowWidth,
        .windowHeight = g_app.windowHeight,
        .requestedInstanceExtensions = {"VK_KHR_surface", "VK_KHR_xcb_surface"},
        .requestedInstanceLayers = {"VK_LAYER_KHRONOS_validation"},
        .requestedDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_NV_MESH_SHADER_EXTENSION_NAME },
        .requestedDeviceFeatures = {SupportedDeviceFeature::eSynchronization2, SupportedDeviceFeature::eDescriptorIndexing, SupportedDeviceFeature::eMeshShadingNV},
        .requestedQueueTypes = {VK_QUEUE_GRAPHICS_BIT},
        .requestedQueuePriorities = { 1.0f },
        .requestedSwapchainImageCount = 2u,
        .requestedSwapchainFormat = VK_FORMAT_R8G8B8A8_SRGB,
        .requestedSwapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR};

    g_vk = vkmInit(initParams);

    setPhysicalDeviceMemoryProperties(g_vk.physicalDeviceMemoryProperties);

    createDesriptorPools();
    createDescriptorSetLayouts();
    createDescriptorSets();
    createRenderPass();
    createFramebuffers();
    createPipelineLayouts();
    createPipelines();
    createCommandPools();
    createCommandBuffers();
    createSynchornizationResources();

    // Staging Buffer 
    VkDeviceSize stagingBufferSize = 50000000;
    createBuffer(g_vk.device, stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, g_vk.buffers[BUFFER_STAGING]);

    // PerFrameUBO Buffer
    createBuffer(g_vk.device, sizeof(PerFrameUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, g_vk.buffers[BUFFER_PER_FRAME_UBO]);

    g_app.projMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -2.0f, 2.0f);

    const PerFrameUBO perFrameUBO {
        .viewMatrix = g_camera.matrix,
        .projMatrix = g_app.projMatrix
    };

    uploadToBuffer(g_vk.device, g_vk.buffers[BUFFER_PER_FRAME_UBO], sizeof(PerFrameUBO), 0, (void*)&perFrameUBO);

    // PerMatUBO Buffer
    createBuffer(g_vk.device, sizeof(PerMatUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, g_vk.buffers[BUFFER_MATERIAL_UBO]);
    g_config.materialAlbedo = glm::vec3(1.0f, 0.0f, 0.0f);
    g_config.materialRoughness = 0.5f;
    glm::vec4 materialAlbedo = glm::vec4(g_config.materialAlbedo, 0.0f);
    glm::vec4 materialRoughness = glm::vec4(g_config.materialRoughness, 0.0f, 0.0f, 0.0f);
    uploadToBuffer(g_vk.device, g_vk.buffers[BUFFER_MATERIAL_UBO], sizeof(glm::vec4), offsetof(PerMatUBO, albedo), (void*)&materialAlbedo);
    uploadToBuffer(g_vk.device, g_vk.buffers[BUFFER_MATERIAL_UBO], sizeof(glm::vec4), offsetof(PerMatUBO, roughness), (void*)&materialRoughness);

    // LightUBO
    createBuffer(g_vk.device, sizeof(LightUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, g_vk.buffers[BUFFER_LIGHT_UBO]);
    g_config.dirLightIntensity = glm::vec3(1.0f, 1.0f, 1.0f);
    g_config.dirLightPosition = glm::vec3(5.0f, 10.0f, 10.0f);
    glm::vec4 dirLightIntensity = glm::vec4(g_config.dirLightIntensity, 0.0f);
    glm::vec4 dirLightPosition = glm::vec4(g_config.dirLightPosition, 0.0f);
    uploadToBuffer(g_vk.device, g_vk.buffers[BUFFER_LIGHT_UBO], sizeof(glm::vec4), offsetof(LightUBO, position), (void*)&dirLightPosition);
    uploadToBuffer(g_vk.device, g_vk.buffers[BUFFER_LIGHT_UBO], sizeof(glm::vec4), offsetof(LightUBO, intensity), (void*)&dirLightIntensity);
    
    // Geometry SSBO
    VkDeviceSize geometrySSBOSize = 50000000;
    createBuffer(g_vk.device, geometrySSBOSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, g_vk.buffers[BUFFER_GEOMETRY_SSBO]);

    // Scene
    MeshBufferData meshVertexData = loadMeshFile("../meshes/sphere.mesh");

    createBuffer(g_vk.device, sizeof(float) * meshVertexData.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, g_vk.buffers[BUFFER_OBJECT_VERTEX]);
    uploadBuffer(g_vk.device, g_vk.commandPools[COMMAND_BUFFER_DEFAULT], g_vk.commandBuffers[COMMAND_BUFFER_DEFAULT], g_vk.queues[QUEUE_GRAPHICS], g_vk.buffers[BUFFER_STAGING], g_vk.buffers[BUFFER_OBJECT_VERTEX], sizeof(float) * meshVertexData.vertices.size(), meshVertexData.vertices.data());
    createBuffer(g_vk.device, sizeof(uint32_t) * meshVertexData.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, g_vk.buffers[BUFFER_OBJECT_INDEX]);
    uploadBuffer(g_vk.device, g_vk.commandPools[COMMAND_BUFFER_DEFAULT], g_vk.commandBuffers[COMMAND_BUFFER_DEFAULT], g_vk.queues[QUEUE_GRAPHICS], g_vk.buffers[BUFFER_STAGING], g_vk.buffers[BUFFER_OBJECT_INDEX], sizeof(uint32_t) * meshVertexData.indices.size(), meshVertexData.indices.data());
    g_app.indexCount[BUFFER_OBJECT_INDEX] = meshVertexData.indices.size();

    updateDescriptorSets();
}

void update()
{
    if (g_camera.dirty)
    {
        uploadToBuffer(g_vk.device, g_vk.buffers[BUFFER_PER_FRAME_UBO], sizeof(glm::mat4), offsetof(PerFrameUBO, viewMatrix), (void*)&g_camera.matrix);
        uploadToBuffer(g_vk.device, g_vk.buffers[BUFFER_PER_FRAME_UBO], sizeof(glm::vec3), offsetof(PerFrameUBO, viewPos), (void*)&g_camera.pos);
    }
}

void gui()
{
    // bool open = true;
    // if (ImGui::Begin("App Config", &open))
    // {

    //     ImGui::End();
    // }
}

void draw()
{
    VK_CHECK(vkAcquireNextImageKHR(g_vk.device, g_vk.swapchain.swapchain, UINT64_MAX, VK_NULL_HANDLE, g_vk.fences[FENCE_IMAGE_ACQUIRE], &g_vk.currentSwapchainImageIdx));
    VK_CHECK(vkWaitForFences(g_vk.device, 1, &g_vk.fences[FENCE_IMAGE_ACQUIRE], VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(g_vk.device, 1, &g_vk.fences[FENCE_IMAGE_ACQUIRE]));

    static const VkCommandBufferBeginInfo commandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    static const std::array<VkClearValue, 2> clearValues {{
        {
            .color = {0.22f, 0.22f, 0.22f, 1.0f},
        },
        {
            .depthStencil = { 1.0f, 0u }
        }
    }};

    const VkRenderPassBeginInfo renderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = g_vk.renderPass,
        .framebuffer = g_vk.framebuffers[g_vk.currentSwapchainImageIdx],
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = g_vk.swapchain.extent},
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };

    vkResetCommandPool(g_vk.device, g_vk.commandPools[COMMAND_POOL_DEFAULT], 0x0);

    VkCommandBuffer commandBuffer = g_vk.commandBuffers[COMMAND_BUFFER_DEFAULT];

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vk.pipeline);

    const std::array<VkDescriptorSet, 2> sets {{
        g_vk.descriptorSets[DESCRIPTOR_SET_FRAME],
        g_vk.descriptorSets[DESCRIPTOR_SET_MATERIAL],
    }};

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_vk.pipelineLayout, 0, sets.size(), sets.data(), 0, nullptr);
    // vkCmdPushConstants(commandBuffer, g_vk_app.pipeline_layout[PIPELINE_DEFAULT], VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4, &g_app.tex_idx);

    static const VkDeviceSize pOffsets = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &g_vk.buffers[BUFFER_OBJECT_VERTEX].buffer, &pOffsets);
    vkCmdBindIndexBuffer(commandBuffer, g_vk.buffers[BUFFER_OBJECT_INDEX].buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, g_app.indexCount[BUFFER_OBJECT_INDEX], 1, 0, 0, 0);

    if (g_app.displayGui)
    {
        gui();
    }

    vkCmdEndRenderPass(commandBuffer);

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    const VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &g_vk.commandBuffers[COMMAND_BUFFER_DEFAULT],
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    VK_CHECK(vkQueueSubmit(g_vk.queues[QUEUE_GRAPHICS], 1, &submitInfo, VK_NULL_HANDLE));

    VK_CHECK(vkQueueWaitIdle(g_vk.queues[QUEUE_GRAPHICS]));

    // Present (wait for graphics work to complete)
    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .swapchainCount = 1,
        .pSwapchains = &g_vk.swapchain.swapchain,
        .pImageIndices = &g_vk.currentSwapchainImageIdx,
        .pResults = nullptr,
    };

    VK_CHECK(vkQueuePresentKHR(g_vk.queues[QUEUE_GRAPHICS], &presentInfo));

    VK_CHECK(vkQueueWaitIdle(g_vk.queues[QUEUE_GRAPHICS]));
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    g_app.window = glfwCreateWindow(g_app.windowWidth, g_app.windowHeight, "Vk-Template", nullptr, nullptr);

    if (g_app.window == nullptr)
    {
        glfwTerminate();
        EXIT("=> Failure <=\n");
    }
    glfwMakeContextCurrent(g_app.window);
    // glfwSetKeyCallback(g_app.window, key_callback);
    glfwSetCursorPosCallback(g_app.window, cursorPositionCallback);
    glfwSetMouseButtonCallback(g_app.window, mouseButtonCallback);

    LOG("-- Begin -- Init\n");
    init();
    LOG("-- End -- Init\n");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(g_app.window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_vk.instance;
    init_info.PhysicalDevice = g_vk.physicalDevice;
    init_info.Device = g_vk.device;
    init_info.QueueFamily = g_vk.queueFamilyIndices[QUEUE_GRAPHICS];
    init_info.Queue = g_vk.queues[QUEUE_GRAPHICS];
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = g_vk.descriptorPools[DESCRIPTOR_POOL_IMGUI];
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = static_cast<uint32_t>(g_vk.swapchain.images.size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&init_info, g_vk.renderPass);

    g_app.initDone = true;

    // Upload Fonts
    {
        // Use any command queue
        VkCommandBuffer command_buffer = g_vk.commandBuffers[COMMAND_BUFFER_DEFAULT];

        const VkCommandBufferBeginInfo command_buffer_begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

        VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VK_CHECK(vkEndCommandBuffer(command_buffer));

        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer};

        VK_CHECK(vkQueueSubmit(g_vk.queues[QUEUE_GRAPHICS], 1, &submit_info, VK_NULL_HANDLE));

        VK_CHECK(vkDeviceWaitIdle(g_vk.device));

        ImGui_ImplVulkan_DestroyFontUploadObjects();

        vkResetCommandPool(g_vk.device, g_vk.commandPools[COMMAND_POOL_DEFAULT], 0x0);
    }

    LOG("-- Begin -- Run\n");

    const glm::vec3 starting_pos{20, 20, 0};

    // // load starting tiles
    // teleport_load(glm::vec2(starting_pos), 8);

    while (!glfwWindowShouldClose(g_app.window))
    {
        glfwPollEvents();

        update();

        draw();

        glfwSwapBuffers(g_app.window);
    }

    VK_CHECK(vkDeviceWaitIdle(g_vk.device));

    LOG("-- End -- Run\n");

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkmDestroy(g_vk);

    glfwDestroyWindow(g_app.window);
    glfwTerminate();

    LOG("-- Release Successful --\n");

    return 0;
}