#include <enDjinn/enj_platform.h>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <vector>

namespace {

enj_host_vid_mode_t g_vid_mode{1280, 960};
std::array<uint8_t, 256> g_dr_packet{};
uint32_t g_current_argb = 0xffffffffu;
pvr_list_t g_current_list = PVR_LIST_OP_POLY;
float g_bg_color[3] = {0.0f, 0.0f, 0.0f};
pvr_sprite_col_t g_sprite_first{};
bool g_has_sprite_first = false;
std::vector<pvr_vertex_t> g_triangle_vertices;
SDL_Window *g_window = nullptr;
VkInstance g_instance = VK_NULL_HANDLE;
VkSurfaceKHR g_surface = VK_NULL_HANDLE;
VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
VkDevice g_device = VK_NULL_HANDLE;
VkQueue g_graphics_queue = VK_NULL_HANDLE;
uint32_t g_graphics_queue_family = 0u;
VkSwapchainKHR g_swapchain = VK_NULL_HANDLE;
VkFormat g_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
VkExtent2D g_extent{1280u, 960u};
std::vector<VkImage> g_swapchain_images;
std::vector<VkImageView> g_swapchain_views;
VkImage g_depth_image = VK_NULL_HANDLE;
VkDeviceMemory g_depth_memory = VK_NULL_HANDLE;
VkImageView g_depth_view = VK_NULL_HANDLE;
VkRenderPass g_render_pass = VK_NULL_HANDLE;
std::vector<VkFramebuffer> g_framebuffers;
VkPipelineLayout g_pipeline_layout = VK_NULL_HANDLE;
VkPipeline g_pipeline = VK_NULL_HANDLE;
VkCommandPool g_command_pool = VK_NULL_HANDLE;
std::vector<VkCommandBuffer> g_command_buffers;
VkSemaphore g_image_available = VK_NULL_HANDLE;
VkSemaphore g_render_finished = VK_NULL_HANDLE;
VkFence g_in_flight = VK_NULL_HANDLE;
VkBuffer g_vertex_buffer = VK_NULL_HANDLE;
VkDeviceMemory g_vertex_memory = VK_NULL_HANDLE;
size_t g_vertex_capacity = 0u;
uint64_t g_presented_frames = 0u;

struct PcVertex {
    float position[3];
    float color[4];
};

struct QueuedPrimitive {
    float x[4];
    float y[4];
    float z[4];
    uint32_t count;
    uint32_t argb;
    pvr_list_t list;
};

std::vector<QueuedPrimitive> g_primitives;

bool create_draw_resources();

void set_window_title(const char *title)
{
    if (g_window != nullptr && title != nullptr) {
        SDL_SetWindowTitle(g_window, title);
    }
}

std::vector<char> read_file(const char *path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        return {};
    }
    const std::streamsize size = file.tellg();
    if (size <= 0) {
        return {};
    }
    std::vector<char> bytes(static_cast<size_t>(size));
    file.seekg(0);
    file.read(bytes.data(), size);
    return bytes;
}

std::vector<char> read_shader_file(const char *name)
{
    const char *roots[] = {
        "integrations/dream_driving/build/pc-endjinn/",
        "build/pc-endjinn/",
    };
    for (const char *root : roots) {
        char path[512];
        std::snprintf(path, sizeof(path), "%s%s", root, name);
        std::vector<char> bytes = read_file(path);
        if (!bytes.empty()) {
            return bytes;
        }
    }
    return {};
}

float clamp01(float value)
{
    return std::min(std::max(value, 0.0f), 1.0f);
}

bool argb_is_road_decal(uint32_t argb)
{
    if ((argb & 0xff000000u) == 0u) {
        return false;
    }
    const uint32_t r = (argb >> 16u) & 0xffu;
    const uint32_t g = (argb >> 8u) & 0xffu;
    const uint32_t b = argb & 0xffu;
    const uint32_t maxc = std::max(r, std::max(g, b));
    const uint32_t minc = std::min(r, std::min(g, b));
    const bool white = minc >= 176u;
    const bool yellow_or_orange = r >= 176u && g >= 132u && b <= 96u;
    const bool skid = maxc <= 96u && maxc >= 18u && maxc - minc <= 32u;
    return white || yellow_or_orange || skid;
}

float pvr_decal_depth_bias(uint32_t argb, float z)
{
    if (!argb_is_road_decal(argb)) {
        return z;
    }
    float bias = 0.000035f + z * 0.00105f;
    if (bias > 0.0011f) {
        bias = 0.0011f;
    }
    return clamp01(z + bias);
}

uint32_t find_memory_type(uint32_t bits, VkMemoryPropertyFlags flags)
{
    VkPhysicalDeviceMemoryProperties props{};
    vkGetPhysicalDeviceMemoryProperties(g_physical_device, &props);
    for (uint32_t i = 0u; i < props.memoryTypeCount; i++) {
        if ((bits & (1u << i)) != 0u &&
            (props.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }
    return 0u;
}

VkShaderModule create_shader_module(const std::vector<char> &bytes)
{
    if (bytes.empty()) {
        return VK_NULL_HANDLE;
    }
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = bytes.size();
    info.pCode = reinterpret_cast<const uint32_t *>(bytes.data());
    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(g_device, &info, nullptr, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

void destroy_frame_resources()
{
    if (g_vertex_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(g_device, g_vertex_buffer, nullptr);
        g_vertex_buffer = VK_NULL_HANDLE;
    }
    if (g_vertex_memory != VK_NULL_HANDLE) {
        vkFreeMemory(g_device, g_vertex_memory, nullptr);
        g_vertex_memory = VK_NULL_HANDLE;
    }
    g_vertex_capacity = 0u;
    if (g_in_flight != VK_NULL_HANDLE) {
        vkDestroyFence(g_device, g_in_flight, nullptr);
        g_in_flight = VK_NULL_HANDLE;
    }
    if (g_render_finished != VK_NULL_HANDLE) {
        vkDestroySemaphore(g_device, g_render_finished, nullptr);
        g_render_finished = VK_NULL_HANDLE;
    }
    if (g_image_available != VK_NULL_HANDLE) {
        vkDestroySemaphore(g_device, g_image_available, nullptr);
        g_image_available = VK_NULL_HANDLE;
    }
    if (!g_command_buffers.empty()) {
        vkFreeCommandBuffers(
            g_device,
            g_command_pool,
            static_cast<uint32_t>(g_command_buffers.size()),
            g_command_buffers.data());
        g_command_buffers.clear();
    }
    if (g_command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(g_device, g_command_pool, nullptr);
        g_command_pool = VK_NULL_HANDLE;
    }
    if (g_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, g_pipeline, nullptr);
        g_pipeline = VK_NULL_HANDLE;
    }
    if (g_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(g_device, g_pipeline_layout, nullptr);
        g_pipeline_layout = VK_NULL_HANDLE;
    }
    for (VkFramebuffer framebuffer : g_framebuffers) {
        vkDestroyFramebuffer(g_device, framebuffer, nullptr);
    }
    g_framebuffers.clear();
    if (g_render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(g_device, g_render_pass, nullptr);
        g_render_pass = VK_NULL_HANDLE;
    }
    if (g_depth_view != VK_NULL_HANDLE) {
        vkDestroyImageView(g_device, g_depth_view, nullptr);
        g_depth_view = VK_NULL_HANDLE;
    }
    if (g_depth_image != VK_NULL_HANDLE) {
        vkDestroyImage(g_device, g_depth_image, nullptr);
        g_depth_image = VK_NULL_HANDLE;
    }
    if (g_depth_memory != VK_NULL_HANDLE) {
        vkFreeMemory(g_device, g_depth_memory, nullptr);
        g_depth_memory = VK_NULL_HANDLE;
    }
    for (VkImageView view : g_swapchain_views) {
        vkDestroyImageView(g_device, view, nullptr);
    }
    g_swapchain_views.clear();
    if (g_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(g_device, g_swapchain, nullptr);
        g_swapchain = VK_NULL_HANDLE;
    }
    g_swapchain_images.clear();
}

bool create_swapchain()
{
    VkSurfaceCapabilitiesKHR caps{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_physical_device, g_surface, &caps) !=
        VK_SUCCESS) {
        return false;
    }
    uint32_t format_count = 0u;
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_physical_device, g_surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_physical_device, g_surface, &format_count, formats.data());
    VkSurfaceFormatKHR chosen = formats.empty()
        ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
        : formats[0];
    for (const VkSurfaceFormatKHR &format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
            chosen = format;
            break;
        }
    }
    g_swapchain_format = chosen.format;
    if (caps.currentExtent.width != UINT32_MAX) {
        g_extent = caps.currentExtent;
    } else {
        int drawable_w = 0;
        int drawable_h = 0;
        SDL_Vulkan_GetDrawableSize(g_window, &drawable_w, &drawable_h);
        g_extent.width = std::min(
            std::max(static_cast<uint32_t>(std::max(drawable_w, 1)), caps.minImageExtent.width),
            caps.maxImageExtent.width);
        g_extent.height = std::min(
            std::max(static_cast<uint32_t>(std::max(drawable_h, 1)), caps.minImageExtent.height),
            caps.maxImageExtent.height);
    }
    uint32_t image_count = caps.minImageCount + 1u;
    if (caps.maxImageCount > 0u && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }
    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = g_surface;
    info.minImageCount = image_count;
    info.imageFormat = chosen.format;
    info.imageColorSpace = chosen.colorSpace;
    info.imageExtent = g_extent;
    info.imageArrayLayers = 1u;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.clipped = VK_TRUE;
    if (vkCreateSwapchainKHR(g_device, &info, nullptr, &g_swapchain) != VK_SUCCESS) {
        return false;
    }
    vkGetSwapchainImagesKHR(g_device, g_swapchain, &image_count, nullptr);
    g_swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(g_device, g_swapchain, &image_count, g_swapchain_images.data());
    g_swapchain_views.resize(g_swapchain_images.size());
    for (size_t i = 0; i < g_swapchain_images.size(); i++) {
        VkImageViewCreateInfo view{};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = g_swapchain_images[i];
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = g_swapchain_format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.levelCount = 1u;
        view.subresourceRange.layerCount = 1u;
        if (vkCreateImageView(g_device, &view, nullptr, &g_swapchain_views[i]) != VK_SUCCESS) {
            return false;
        }
    }
    return true;
}

bool recreate_draw_resources()
{
    if (g_device == VK_NULL_HANDLE) {
        return false;
    }
    vkDeviceWaitIdle(g_device);
    destroy_frame_resources();
    return create_draw_resources();
}

bool create_depth_resources()
{
    VkImageCreateInfo image{};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.extent = {g_extent.width, g_extent.height, 1u};
    image.mipLevels = 1u;
    image.arrayLayers = 1u;
    image.format = VK_FORMAT_D32_SFLOAT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(g_device, &image, nullptr, &g_depth_image) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(g_device, g_depth_image, &req);
    VkMemoryAllocateInfo mem{};
    mem.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem.allocationSize = req.size;
    mem.memoryTypeIndex = find_memory_type(
        req.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(g_device, &mem, nullptr, &g_depth_memory) != VK_SUCCESS) {
        return false;
    }
    if (vkBindImageMemory(g_device, g_depth_image, g_depth_memory, 0u) != VK_SUCCESS) {
        return false;
    }

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = g_depth_image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = VK_FORMAT_D32_SFLOAT;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view.subresourceRange.levelCount = 1u;
    view.subresourceRange.layerCount = 1u;
    return vkCreateImageView(g_device, &view, nullptr, &g_depth_view) == VK_SUCCESS;
}

bool create_render_pipeline()
{
    VkAttachmentDescription color{};
    color.format = g_swapchain_format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentDescription depth{};
    depth.format = VK_FORMAT_D32_SFLOAT;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference color_ref{};
    color_ref.attachment = 0u;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depth_ref{};
    depth_ref.attachment = 1u;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1u;
    subpass.pColorAttachments = &color_ref;
    subpass.pDepthStencilAttachment = &depth_ref;
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0u;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    const VkAttachmentDescription attachments[2] = {color, depth};
    VkRenderPassCreateInfo render_pass{};
    render_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass.attachmentCount = 2u;
    render_pass.pAttachments = attachments;
    render_pass.subpassCount = 1u;
    render_pass.pSubpasses = &subpass;
    render_pass.dependencyCount = 1u;
    render_pass.pDependencies = &dep;
    if (vkCreateRenderPass(g_device, &render_pass, nullptr, &g_render_pass) != VK_SUCCESS) {
        return false;
    }
    g_framebuffers.resize(g_swapchain_views.size());
    for (size_t i = 0; i < g_swapchain_views.size(); i++) {
        VkImageView attachments_for_fb[2] = {g_swapchain_views[i], g_depth_view};
        VkFramebufferCreateInfo fb{};
        fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb.renderPass = g_render_pass;
        fb.attachmentCount = 2u;
        fb.pAttachments = attachments_for_fb;
        fb.width = g_extent.width;
        fb.height = g_extent.height;
        fb.layers = 1u;
        if (vkCreateFramebuffer(g_device, &fb, nullptr, &g_framebuffers[i]) != VK_SUCCESS) {
            return false;
        }
    }

    std::vector<char> vert = read_shader_file("flat.vert.spv");
    std::vector<char> frag = read_shader_file("flat.frag.spv");
    VkShaderModule vert_module = create_shader_module(vert);
    VkShaderModule frag_module = create_shader_module(frag);
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
        std::fprintf(stderr, "pc-enDjinn: missing pc-endjinn flat shaders\n");
        return false;
    }
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert_module;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag_module;
    stages[1].pName = "main";
    VkVertexInputBindingDescription binding{};
    binding.binding = 0u;
    binding.stride = sizeof(PcVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0] = {0u, 0u, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PcVertex, position)};
    attrs[1] = {1u, 0u, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(PcVertex, color)};
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1u;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = 2u;
    vertex_input.pVertexAttributeDescriptions = attrs;
    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport viewport{0.0f, 0.0f, (float)g_extent.width, (float)g_extent.height, 0.0f, 1.0f};
    VkRect2D scissor{{0, 0}, g_extent};
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1u;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1u;
    viewport_state.pScissors = &scissor;
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth_state{};
    depth_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_state.depthTestEnable = VK_TRUE;
    depth_state.depthWriteEnable = VK_TRUE;
    depth_state.depthCompareOp = VK_COMPARE_OP_GREATER;
    VkPipelineColorBlendAttachmentState blend_att{};
    blend_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1u;
    blend.pAttachments = &blend_att;
    VkPipelineLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(g_device, &layout, nullptr, &g_pipeline_layout) != VK_SUCCESS) {
        return false;
    }
    VkGraphicsPipelineCreateInfo pipe{};
    pipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipe.stageCount = 2u;
    pipe.pStages = stages;
    pipe.pVertexInputState = &vertex_input;
    pipe.pInputAssemblyState = &assembly;
    pipe.pViewportState = &viewport_state;
    pipe.pRasterizationState = &raster;
    pipe.pMultisampleState = &ms;
    pipe.pDepthStencilState = &depth_state;
    pipe.pColorBlendState = &blend;
    pipe.layout = g_pipeline_layout;
    pipe.renderPass = g_render_pass;
    const bool ok = vkCreateGraphicsPipelines(
        g_device, VK_NULL_HANDLE, 1u, &pipe, nullptr, &g_pipeline) == VK_SUCCESS;
    vkDestroyShaderModule(g_device, frag_module, nullptr);
    vkDestroyShaderModule(g_device, vert_module, nullptr);
    return ok;
}

bool create_draw_resources()
{
    if (!create_swapchain() || !create_depth_resources() || !create_render_pipeline()) {
        return false;
    }
    VkCommandPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool.queueFamilyIndex = g_graphics_queue_family;
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(g_device, &pool, nullptr, &g_command_pool) != VK_SUCCESS) {
        return false;
    }
    g_command_buffers.resize(g_framebuffers.size());
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = g_command_pool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = static_cast<uint32_t>(g_command_buffers.size());
    if (vkAllocateCommandBuffers(g_device, &alloc, g_command_buffers.data()) != VK_SUCCESS) {
        return false;
    }
    VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    return vkCreateSemaphore(g_device, &sem, nullptr, &g_image_available) == VK_SUCCESS &&
        vkCreateSemaphore(g_device, &sem, nullptr, &g_render_finished) == VK_SUCCESS &&
        vkCreateFence(g_device, &fence, nullptr, &g_in_flight) == VK_SUCCESS;
}

bool ensure_vertex_buffer(size_t vertex_count)
{
    const size_t bytes = vertex_count * sizeof(PcVertex);
    if (bytes == 0u) {
        return true;
    }
    if (g_vertex_buffer != VK_NULL_HANDLE && g_vertex_capacity >= bytes) {
        return true;
    }
    if (g_vertex_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(g_device, g_vertex_buffer, nullptr);
        vkFreeMemory(g_device, g_vertex_memory, nullptr);
    }
    VkBufferCreateInfo buffer{};
    buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer.size = bytes;
    buffer.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(g_device, &buffer, nullptr, &g_vertex_buffer) != VK_SUCCESS) {
        return false;
    }
    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(g_device, g_vertex_buffer, &req);
    VkMemoryAllocateInfo mem{};
    mem.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem.allocationSize = req.size;
    mem.memoryTypeIndex = find_memory_type(
        req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(g_device, &mem, nullptr, &g_vertex_memory) != VK_SUCCESS) {
        return false;
    }
    vkBindBufferMemory(g_device, g_vertex_buffer, g_vertex_memory, 0u);
    g_vertex_capacity = bytes;
    return true;
}

PcVertex make_vertex(float x, float y, float z, uint32_t argb)
{
    const float half_w = (float)g_vid_mode.width;
    const float half_h = (float)g_vid_mode.height * 0.5f;
    const float a = (float)((argb >> 24) & 0xffu) / 255.0f;
    const float r = (float)((argb >> 16) & 0xffu) / 255.0f;
    const float g = (float)((argb >> 8) & 0xffu) / 255.0f;
    const float b = (float)(argb & 0xffu) / 255.0f;
    PcVertex vertex{};
    vertex.position[0] = x / half_w - 1.0f;
    vertex.position[1] = y / half_h - 1.0f;
    vertex.position[2] = pvr_decal_depth_bias(argb, clamp01(z * 0.25f));
    vertex.color[0] = r;
    vertex.color[1] = g;
    vertex.color[2] = b;
    vertex.color[3] = a;
    return vertex;
}

std::vector<PcVertex> build_vertices()
{
    std::vector<PcVertex> out;
    out.reserve(g_primitives.size() * 6u);
    const auto emit = [&](const QueuedPrimitive &p, uint32_t i) {
        out.push_back(make_vertex(p.x[i], p.y[i], p.z[i], p.argb));
    };
    for (const QueuedPrimitive &p : g_primitives) {
        if (p.count == 3u) {
            emit(p, 0u); emit(p, 1u); emit(p, 2u);
        } else if (p.count == 4u) {
            emit(p, 0u); emit(p, 1u); emit(p, 2u);
            emit(p, 0u); emit(p, 2u); emit(p, 3u);
        }
    }
    return out;
}

bool draw_frame(const std::vector<PcVertex> &vertices)
{
    if (g_device == VK_NULL_HANDLE || g_swapchain == VK_NULL_HANDLE ||
        g_pipeline == VK_NULL_HANDLE || g_command_buffers.empty()) {
        set_window_title("pc-enDjinn - draw unavailable");
        return false;
    }
    vkWaitForFences(g_device, 1u, &g_in_flight, VK_TRUE, UINT64_MAX);
    vkResetFences(g_device, 1u, &g_in_flight);

    uint32_t image_index = 0u;
    VkResult acquire = vkAcquireNextImageKHR(
        g_device, g_swapchain, UINT64_MAX, g_image_available, VK_NULL_HANDLE, &image_index);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreate_draw_resources();
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        set_window_title("pc-enDjinn - vkAcquireNextImageKHR failed");
        return false;
    }

    if (!ensure_vertex_buffer(vertices.size())) {
        set_window_title("pc-enDjinn - vertex buffer failed");
        return false;
    }
    if (!vertices.empty()) {
        void *mapped = nullptr;
        const VkDeviceSize bytes = static_cast<VkDeviceSize>(vertices.size() * sizeof(PcVertex));
        if (vkMapMemory(g_device, g_vertex_memory, 0u, bytes, 0u, &mapped) != VK_SUCCESS) {
            set_window_title("pc-enDjinn - vertex map failed");
            return false;
        }
        std::memcpy(mapped, vertices.data(), static_cast<size_t>(bytes));
        vkUnmapMemory(g_device, g_vertex_memory);
    }

    VkCommandBuffer cmd = g_command_buffers[image_index];
    vkResetCommandBuffer(cmd, 0u);
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &begin) != VK_SUCCESS) {
        set_window_title("pc-enDjinn - command begin failed");
        return false;
    }
    VkClearValue clears[2]{};
    clears[0].color.float32[0] = g_bg_color[0];
    clears[0].color.float32[1] = g_bg_color[1];
    clears[0].color.float32[2] = g_bg_color[2];
    clears[0].color.float32[3] = 1.0f;
    clears[1].depthStencil = {0.0f, 0u};
    VkRenderPassBeginInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass.renderPass = g_render_pass;
    pass.framebuffer = g_framebuffers[image_index];
    pass.renderArea.extent = g_extent;
    pass.clearValueCount = 2u;
    pass.pClearValues = clears;
    vkCmdBeginRenderPass(cmd, &pass, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);
    if (!vertices.empty()) {
        VkDeviceSize offset = 0u;
        vkCmdBindVertexBuffers(cmd, 0u, 1u, &g_vertex_buffer, &offset);
        vkCmdDraw(cmd, static_cast<uint32_t>(vertices.size()), 1u, 0u, 0u);
    }
    vkCmdEndRenderPass(cmd);
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        set_window_title("pc-enDjinn - command end failed");
        return false;
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1u;
    submit.pWaitSemaphores = &g_image_available;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1u;
    submit.pSignalSemaphores = &g_render_finished;
    if (vkQueueSubmit(g_graphics_queue, 1u, &submit, g_in_flight) != VK_SUCCESS) {
        set_window_title("pc-enDjinn - queue submit failed");
        return false;
    }

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1u;
    present.pWaitSemaphores = &g_render_finished;
    present.swapchainCount = 1u;
    present.pSwapchains = &g_swapchain;
    present.pImageIndices = &image_index;
    const VkResult presented = vkQueuePresentKHR(g_graphics_queue, &present);
    if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR) {
        (void)recreate_draw_resources();
    }
    return true;
}

void queue_triangle(const pvr_vertex_t &a, const pvr_vertex_t &b, const pvr_vertex_t &c)
{
    QueuedPrimitive primitive{};
    primitive.count = 3u;
    primitive.argb = c.argb != 0u ? c.argb : g_current_argb;
    primitive.list = g_current_list;
    primitive.x[0] = a.x; primitive.y[0] = a.y; primitive.z[0] = a.z;
    primitive.x[1] = b.x; primitive.y[1] = b.y; primitive.z[1] = b.z;
    primitive.x[2] = c.x; primitive.y[2] = c.y; primitive.z[2] = c.z;
    g_primitives.push_back(primitive);
}

void queue_sprite_second_half(const void *ptr)
{
    const float *tail = static_cast<const float *>(ptr);
    QueuedPrimitive primitive{};
    primitive.count = 4u;
    primitive.argb = g_current_argb;
    primitive.list = g_current_list;
    primitive.x[0] = g_sprite_first.ax;
    primitive.y[0] = g_sprite_first.ay;
    primitive.z[0] = g_sprite_first.az;
    primitive.x[1] = g_sprite_first.bx;
    primitive.y[1] = g_sprite_first.by;
    primitive.z[1] = g_sprite_first.bz;
    primitive.x[2] = g_sprite_first.cx;
    primitive.y[2] = tail[0];
    primitive.z[2] = tail[1];
    primitive.x[3] = tail[2];
    primitive.y[3] = tail[3];
    const uint32_t *tail_words = static_cast<const uint32_t *>(ptr);
    primitive.z[3] = tail_words[5] == 0x50435a44u
        ? tail[4]
        : g_sprite_first.bz + tail[1] - g_sprite_first.az;
    g_primitives.push_back(primitive);
    g_has_sprite_first = false;
}

bool has_device_extension(VkPhysicalDevice device, const char *name)
{
    uint32_t count = 0u;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS) {
        return false;
    }
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) !=
        VK_SUCCESS) {
        return false;
    }
    for (const VkExtensionProperties &extension : extensions) {
        if (std::strcmp(extension.extensionName, name) == 0) {
            return true;
        }
    }
    return false;
}

bool has_instance_extension(const char *name)
{
    uint32_t count = 0u;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS) {
        return false;
    }
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) !=
        VK_SUCCESS) {
        return false;
    }
    for (const VkExtensionProperties &extension : extensions) {
        if (std::strcmp(extension.extensionName, name) == 0) {
            return true;
        }
    }
    return false;
}

bool choose_graphics_device()
{
    uint32_t device_count = 0u;
    if (vkEnumeratePhysicalDevices(g_instance, &device_count, nullptr) != VK_SUCCESS ||
        device_count == 0u) {
        return false;
    }
    std::vector<VkPhysicalDevice> devices(device_count);
    if (vkEnumeratePhysicalDevices(g_instance, &device_count, devices.data()) != VK_SUCCESS) {
        return false;
    }
    for (VkPhysicalDevice device : devices) {
        uint32_t queue_count = 0u;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, nullptr);
        std::vector<VkQueueFamilyProperties> queues(queue_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queues.data());
        for (uint32_t i = 0u; i < queue_count; i++) {
            VkBool32 present_supported = VK_FALSE;
            (void)vkGetPhysicalDeviceSurfaceSupportKHR(device, i, g_surface, &present_supported);
            if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u && present_supported) {
                g_physical_device = device;
                g_graphics_queue_family = i;
                return true;
            }
        }
    }
    return false;
}

bool create_vulkan_device()
{
    if (!choose_graphics_device()) {
        return false;
    }
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = g_graphics_queue_family;
    queue_info.queueCount = 1u;
    queue_info.pQueuePriorities = &priority;

    std::vector<const char *> extensions;
    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    if (has_device_extension(g_physical_device, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }
#endif

    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = 1u;
    info.pQueueCreateInfos = &queue_info;
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();
    if (vkCreateDevice(g_physical_device, &info, nullptr, &g_device) != VK_SUCCESS) {
        return false;
    }
    vkGetDeviceQueue(g_device, g_graphics_queue_family, 0u, &g_graphics_queue);
    return true;
}

bool create_vulkan_context()
{
    if (g_instance != VK_NULL_HANDLE) {
        return true;
    }
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "pc-enDjinn: SDL_InitSubSystem failed: %s\n", SDL_GetError());
        return false;
    }
    g_window = SDL_CreateWindow(
        "pc-enDjinn",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        g_vid_mode.width,
        g_vid_mode.height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (g_window == nullptr) {
        std::fprintf(stderr, "pc-enDjinn: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    set_window_title("pc-enDjinn - creating Vulkan instance");

    unsigned int sdl_extension_count = 0u;
    if (!SDL_Vulkan_GetInstanceExtensions(g_window, &sdl_extension_count, nullptr)) {
        std::fprintf(stderr, "pc-enDjinn: SDL_Vulkan_GetInstanceExtensions failed: %s\n",
            SDL_GetError());
        return false;
    }
    std::vector<const char *> extensions(sdl_extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(g_window, &sdl_extension_count, extensions.data())) {
        std::fprintf(stderr, "pc-enDjinn: SDL_Vulkan_GetInstanceExtensions failed: %s\n",
            SDL_GetError());
        return false;
    }

    VkInstanceCreateFlags flags = 0u;
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    if (has_instance_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "pc-enDjinn";
    app.pEngineName = "enDjinn";
    app.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.flags = flags;
    instance_info.pApplicationInfo = &app;
    instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();
    const VkResult instance_result = vkCreateInstance(&instance_info, nullptr, &g_instance);
    if (instance_result != VK_SUCCESS) {
        std::fprintf(stderr, "pc-enDjinn: vkCreateInstance failed: %d\n",
            static_cast<int>(instance_result));
        char title[128];
        std::snprintf(
            title,
            sizeof(title),
            "pc-enDjinn - vkCreateInstance failed %d",
            static_cast<int>(instance_result));
        set_window_title(title);
        return false;
    }
    if (!SDL_Vulkan_CreateSurface(g_window, g_instance, &g_surface)) {
        std::fprintf(stderr, "pc-enDjinn: SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
        set_window_title("pc-enDjinn - SDL_Vulkan_CreateSurface failed");
        return false;
    }
    if (!create_vulkan_device()) {
        std::fprintf(stderr, "pc-enDjinn: Vulkan device creation failed\n");
        set_window_title("pc-enDjinn - Vulkan device creation failed");
        return false;
    }
    if (!create_draw_resources()) {
        std::fprintf(stderr, "pc-enDjinn: Vulkan draw resource creation failed\n");
        set_window_title("pc-enDjinn - Vulkan draw resource creation failed");
        return false;
    }
    set_window_title("pc-enDjinn - Vulkan ready");
    return true;
}

}  // namespace

extern "C" {

enj_host_vid_mode_t *vid_mode = &g_vid_mode;

uint64_t timer_ns_gettime64(void)
{
    const uint64_t counter = SDL_GetPerformanceCounter();
    const uint64_t frequency = SDL_GetPerformanceFrequency();
    if (frequency == 0u) {
        return 0u;
    }
    return (counter * 1000000000ull) / frequency;
}

void vid_border_color(uint8_t r, uint8_t g, uint8_t b)
{
    (void)r;
    (void)g;
    (void)b;
}

void vid_set_mode(vid_display_mode_generic_t display_mode, vid_pixel_mode_t pixel_mode)
{
    (void)display_mode;
    (void)pixel_mode;
    g_vid_mode.width = 1280;
    g_vid_mode.height = 960;
}

void pvr_init(const pvr_init_params_t *params)
{
    (void)params;
    if (!create_vulkan_context()) {
        std::fprintf(stderr, "pc-enDjinn: pvr_init failed\n");
    }
}

void pvr_shutdown(void)
{
    if (g_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(g_device);
        destroy_frame_resources();
        vkDestroyDevice(g_device, nullptr);
        g_device = VK_NULL_HANDLE;
    }
    g_graphics_queue = VK_NULL_HANDLE;
    g_physical_device = VK_NULL_HANDLE;
    if (g_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(g_instance, g_surface, nullptr);
        g_surface = VK_NULL_HANDLE;
    }
    if (g_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(g_instance, nullptr);
        g_instance = VK_NULL_HANDLE;
    }
    if (g_window != nullptr) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
}

void pvr_set_bg_color(float r, float g, float b)
{
    g_bg_color[0] = clamp01(r);
    g_bg_color[1] = clamp01(g);
    g_bg_color[2] = clamp01(b);
}

void pvr_wait_ready(void) {}
void pvr_scene_begin(void)
{
    g_primitives.clear();
    g_triangle_vertices.clear();
    g_has_sprite_first = false;
}

void pvr_scene_finish(void)
{
    if (g_device == VK_NULL_HANDLE) {
        return;
    }
    const std::vector<PcVertex> vertices = build_vertices();
    (void)draw_frame(vertices);
    g_presented_frames++;
    if (g_window != nullptr && (g_presented_frames <= 3u || (g_presented_frames % 30u) == 0u)) {
        char title[192];
        std::snprintf(
            title,
            sizeof(title),
            "pc-enDjinn - pvr primitives %zu - vk verts %zu",
            g_primitives.size(),
            vertices.size());
        SDL_SetWindowTitle(g_window, title);
    }
}

void pvr_list_begin(pvr_list_t list) { g_current_list = list; }
void pvr_list_finish(void) {}
void pvr_wait_render_done(void) {}
void pvr_set_pal_format(pvr_palfmt_t mode) { (void)mode; }

void pvr_fog_table_color(float a, float r, float g, float b)
{
    (void)a;
    (void)r;
    (void)g;
    (void)b;
}

void pvr_fog_table_linear(float start, float end)
{
    (void)start;
    (void)end;
}

void *pvr_dr_target(void)
{
    if (!g_has_sprite_first) {
        std::memset(g_dr_packet.data(), 0, g_dr_packet.size());
        return g_dr_packet.data();
    }
    std::memset(g_dr_packet.data() + 32u, 0, g_dr_packet.size() - 32u);
    return g_dr_packet.data() + 32u;
}

void pvr_dr_commit(void *ptr)
{
    if (ptr == nullptr) {
        return;
    }

    const uint32_t flags = *static_cast<const uint32_t *>(ptr);
    if (g_has_sprite_first) {
        queue_sprite_second_half(ptr);
        return;
    }

    if (flags == PVR_CMD_VERTEX || flags == PVR_CMD_VERTEX_EOL) {
        if (flags == PVR_CMD_VERTEX_EOL && g_triangle_vertices.empty()) {
            std::memcpy(&g_sprite_first, ptr, sizeof(g_sprite_first));
            g_has_sprite_first = true;
            return;
        }

        const pvr_vertex_t *vertex = static_cast<const pvr_vertex_t *>(ptr);
        g_triangle_vertices.push_back(*vertex);
        if (flags == PVR_CMD_VERTEX_EOL) {
            if (g_triangle_vertices.size() >= 3u) {
                const size_t n = g_triangle_vertices.size();
                queue_triangle(
                    g_triangle_vertices[n - 3u],
                    g_triangle_vertices[n - 2u],
                    g_triangle_vertices[n - 1u]);
            }
            g_triangle_vertices.clear();
        }
        return;
    }

    const pvr_sprite_hdr_t *header = static_cast<const pvr_sprite_hdr_t *>(ptr);
    if (header->argb != 0u) {
        g_current_argb = header->argb;
    }
}

void pc_endjinn_platform_set_video_size(uint32_t width, uint32_t height)
{
    g_vid_mode.width = width > 0u ? static_cast<int>(width) : 1;
    g_vid_mode.height = height > 0u ? static_cast<int>(height) : 1;
}

}  // extern "C"
