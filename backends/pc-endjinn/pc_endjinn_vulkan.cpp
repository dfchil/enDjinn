#include <kos.h>

#include "pc_endjinn_pvr.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

extern "C" void pc_endjinn_input_request_quit(void);
extern "C" void pc_endjinn_input_shutdown(void);

namespace {

vid_mode_t g_vid_mode{640, 480};
float g_bg_color[3] = {0.0f, 0.0f, 0.0f};
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
VkRect2D g_content_rect{{0, 0}, {1280u, 960u}};
std::vector<VkImage> g_swapchain_images;
std::vector<VkImageView> g_swapchain_views;
VkImage g_depth_image = VK_NULL_HANDLE;
VkDeviceMemory g_depth_memory = VK_NULL_HANDLE;
VkImageView g_depth_view = VK_NULL_HANDLE;
VkFormat g_depth_format = VK_FORMAT_UNDEFINED;
VkRenderPass g_render_pass = VK_NULL_HANDLE;
std::vector<VkFramebuffer> g_framebuffers;
VkPipelineLayout g_pipeline_layout = VK_NULL_HANDLE;
VkPipeline g_opaque_pipeline = VK_NULL_HANDLE;
VkPipeline g_punch_through_pipeline = VK_NULL_HANDLE;
VkPipeline g_translucent_pipeline = VK_NULL_HANDLE;
VkPipeline g_modifier_volume_pipeline = VK_NULL_HANDLE;
VkPipeline g_modifier_exclude_pipeline = VK_NULL_HANDLE;
VkPipeline g_modifier_pipeline = VK_NULL_HANDLE;
VkDescriptorSetLayout g_texture_set_layout = VK_NULL_HANDLE;
VkDescriptorPool g_descriptor_pool = VK_NULL_HANDLE;
VkCommandPool g_command_pool = VK_NULL_HANDLE;
std::vector<VkCommandBuffer> g_command_buffers;
VkSemaphore g_image_available = VK_NULL_HANDLE;
VkSemaphore g_render_finished = VK_NULL_HANDLE;
VkFence g_in_flight = VK_NULL_HANDLE;
VkBuffer g_vertex_buffer = VK_NULL_HANDLE;
VkDeviceMemory g_vertex_memory = VK_NULL_HANDLE;
size_t g_vertex_capacity = 0u;
uint64_t g_presented_frames = 0u;
bool g_translucent_autosort = true;
bool g_fsaa_enabled = false;
bool g_fullscreen_toggle_requested = false;
int g_last_window_width = 1280;
int g_last_window_height = 960;

struct PcVertex {
    float position[3];
    float color[4];
    float uv[2];
};

using pc_endjinn_pvr::QueuedPrimitive;

struct TexturePush {
    uint32_t indexed;
    uint32_t palette_base;
    uint32_t filter_mode;
    uint32_t unused;
};

struct DrawBatch {
    pvr_list_t list;
    uint32_t first_vertex;
    uint32_t vertex_count;
    bool textured;
    pvr_ptr_t texture;
    uint32_t texture_format;
    uint32_t texture_width;
    uint32_t texture_height;
    pvr_filter_mode_t texture_filter;
    uint32_t palette_base;
    bool modifier;
    bool modifier_volume;
    uint32_t modifier_mode;
};

struct FrameDrawData {
    std::vector<PcVertex> vertices;
    std::vector<DrawBatch> batches;
};

struct GpuImage {
    VkImage image{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkImageView view{VK_NULL_HANDLE};
    VkSampler sampler{VK_NULL_HANDLE};
    uint32_t width{};
    uint32_t height{};
    uint32_t mip_count{};
    VkFormat format{VK_FORMAT_UNDEFINED};
};

struct GpuTexture {
    pvr_ptr_t source{};
    uint32_t source_format{};
    uint32_t width{};
    uint32_t height{};
    pvr_filter_mode_t filter{PVR_FILTER_NEAREST};
    uint64_t revision{};
    bool indexed{};
    uint32_t palette_base{};
    GpuImage image;
    VkDescriptorSet descriptor{VK_NULL_HANDLE};
};

GpuImage g_white_texture;
GpuImage g_default_index_texture;
GpuImage g_palette_texture;
std::vector<GpuTexture> g_texture_cache;
VkDescriptorSet g_default_texture_descriptor = VK_NULL_HANDLE;
uint64_t g_uploaded_palette_revision = 0u;

bool create_draw_resources();

int SDLCALL pc_endjinn_event_watch(void *, SDL_Event *event)
{
    if (event != nullptr &&
        (event->type == SDL_QUIT ||
         (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE))) {
        pc_endjinn_input_request_quit();
    }
    if (event != nullptr && event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        const bool alt_enter = event->key.keysym.sym == SDLK_RETURN &&
            (event->key.keysym.mod & KMOD_ALT) != 0;
        if (event->key.keysym.sym == SDLK_F11 || alt_enter) {
            g_fullscreen_toggle_requested = true;
        }
    }
    return 1;
}

void update_content_rect()
{
    const uint64_t width = g_extent.width;
    const uint64_t height = g_extent.height;
    uint32_t content_width = g_extent.width;
    uint32_t content_height = g_extent.height;
    if (width * 3u > height * 4u) {
        content_width = static_cast<uint32_t>(height * 4u / 3u);
    } else if (width * 3u < height * 4u) {
        content_height = static_cast<uint32_t>(width * 3u / 4u);
    }
    g_content_rect.offset.x = static_cast<int32_t>((g_extent.width - content_width) / 2u);
    g_content_rect.offset.y = static_cast<int32_t>((g_extent.height - content_height) / 2u);
    g_content_rect.extent = {content_width, content_height};
}

void update_window_mode()
{
    if (g_window == nullptr) {
        return;
    }
    if (g_fullscreen_toggle_requested) {
        const Uint32 flags = SDL_GetWindowFlags(g_window);
        const bool fullscreen = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0u;
        (void)SDL_SetWindowFullscreen(
            g_window, fullscreen ? 0u : SDL_WINDOW_FULLSCREEN_DESKTOP);
        g_fullscreen_toggle_requested = false;
        return;
    }
    if ((SDL_GetWindowFlags(g_window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0u) {
        return;
    }

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(g_window, &width, &height);
    if (width <= 0 || height <= 0) {
        return;
    }
    if (std::abs(width * 3 - height * 4) > 4) {
        const int width_delta = std::abs(width - g_last_window_width);
        const int height_delta = std::abs(height - g_last_window_height);
        if (width_delta >= height_delta) {
            height = std::max(1, width * 3 / 4);
        } else {
            width = std::max(1, height * 4 / 3);
        }
        SDL_SetWindowSize(g_window, width, height);
    }
    g_last_window_width = width;
    g_last_window_height = height;
}

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
        "",
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

void destroy_gpu_image(GpuImage &texture)
{
    if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(g_device, texture.sampler, nullptr);
    }
    if (texture.view != VK_NULL_HANDLE) {
        vkDestroyImageView(g_device, texture.view, nullptr);
    }
    if (texture.image != VK_NULL_HANDLE) {
        vkDestroyImage(g_device, texture.image, nullptr);
    }
    if (texture.memory != VK_NULL_HANDLE) {
        vkFreeMemory(g_device, texture.memory, nullptr);
    }
    texture = {};
}

void destroy_texture_resources()
{
    for (GpuTexture &texture : g_texture_cache) {
        destroy_gpu_image(texture.image);
    }
    g_texture_cache.clear();
    g_default_texture_descriptor = VK_NULL_HANDLE;
    destroy_gpu_image(g_white_texture);
    destroy_gpu_image(g_default_index_texture);
    destroy_gpu_image(g_palette_texture);
    g_uploaded_palette_revision = 0u;
    if (g_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(g_device, g_descriptor_pool, nullptr);
        g_descriptor_pool = VK_NULL_HANDLE;
    }
}

bool create_staging_buffer(VkDeviceSize size, VkBuffer &buffer,
                           VkDeviceMemory &memory)
{
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(g_device, &info, nullptr, &buffer) != VK_SUCCESS) {
        return false;
    }
    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(g_device, buffer, &req);
    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = find_memory_type(
        req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(g_device, &alloc, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(g_device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return false;
    }
    return vkBindBufferMemory(g_device, buffer, memory, 0u) == VK_SUCCESS;
}

bool upload_gpu_image(GpuImage &texture, VkFormat format,
                      const std::vector<pc_endjinn_pvr::DecodedMip> &mips,
                      pvr_filter_mode_t filter)
{
    if (mips.empty() || mips[0].pixels.empty()) {
        return false;
    }
    size_t byte_count = 0u;
    for (const auto &mip : mips) {
        byte_count += mip.pixels.size();
    }
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    if (!create_staging_buffer(byte_count, staging, staging_memory)) {
        return false;
    }
    void *mapped = nullptr;
    if (vkMapMemory(g_device, staging_memory, 0u, byte_count, 0u, &mapped) != VK_SUCCESS) {
        vkDestroyBuffer(g_device, staging, nullptr);
        vkFreeMemory(g_device, staging_memory, nullptr);
        return false;
    }
    size_t offset = 0u;
    std::vector<VkBufferImageCopy> regions;
    regions.reserve(mips.size());
    for (size_t level = 0u; level < mips.size(); level++) {
        const auto &mip = mips[level];
        std::memcpy(static_cast<uint8_t *>(mapped) + offset,
                    mip.pixels.data(), mip.pixels.size());
        VkBufferImageCopy region{};
        region.bufferOffset = offset;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = static_cast<uint32_t>(level);
        region.imageSubresource.layerCount = 1u;
        region.imageExtent = {mip.width, mip.height, 1u};
        regions.push_back(region);
        offset += mip.pixels.size();
    }
    vkUnmapMemory(g_device, staging_memory);

    const bool existing = texture.image != VK_NULL_HANDLE;
    if (!existing) {
        VkImageCreateInfo image{};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.extent = {mips[0].width, mips[0].height, 1u};
        image.mipLevels = static_cast<uint32_t>(mips.size());
        image.arrayLayers = 1u;
        image.format = format;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateImage(g_device, &image, nullptr, &texture.image) != VK_SUCCESS) {
            vkDestroyBuffer(g_device, staging, nullptr);
            vkFreeMemory(g_device, staging_memory, nullptr);
            return false;
        }
        VkMemoryRequirements req{};
        vkGetImageMemoryRequirements(g_device, texture.image, &req);
        VkMemoryAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize = req.size;
        alloc.memoryTypeIndex = find_memory_type(
            req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(g_device, &alloc, nullptr, &texture.memory) != VK_SUCCESS ||
            vkBindImageMemory(g_device, texture.image, texture.memory, 0u) != VK_SUCCESS) {
            vkDestroyBuffer(g_device, staging, nullptr);
            vkFreeMemory(g_device, staging_memory, nullptr);
            destroy_gpu_image(texture);
            return false;
        }
        texture.width = mips[0].width;
        texture.height = mips[0].height;
        texture.mip_count = static_cast<uint32_t>(mips.size());
        texture.format = format;
    }

    VkCommandBufferAllocateInfo command_alloc{};
    command_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_alloc.commandPool = g_command_pool;
    command_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_alloc.commandBufferCount = 1u;
    VkCommandBuffer command = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(g_device, &command_alloc, &command) != VK_SUCCESS) {
        vkDestroyBuffer(g_device, staging, nullptr);
        vkFreeMemory(g_device, staging_memory, nullptr);
        return false;
    }
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command, &begin);
    VkImageMemoryBarrier to_transfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    to_transfer.oldLayout = existing ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                     : VK_IMAGE_LAYOUT_UNDEFINED;
    to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_transfer.srcAccessMask = existing ? VK_ACCESS_SHADER_READ_BIT : 0u;
    to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_transfer.image = texture.image;
    to_transfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    to_transfer.subresourceRange.levelCount = texture.mip_count;
    to_transfer.subresourceRange.layerCount = 1u;
    vkCmdPipelineBarrier(
        command,
        existing ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, nullptr, 0u, nullptr, 1u, &to_transfer);
    vkCmdCopyBufferToImage(command, staging, texture.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(regions.size()), regions.data());
    VkImageMemoryBarrier to_shader = to_transfer;
    to_shader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_shader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    to_shader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_shader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u, 0u, nullptr,
                         0u, nullptr, 1u, &to_shader);
    vkEndCommandBuffer(command);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &command;
    const bool submitted =
        vkQueueSubmit(g_graphics_queue, 1u, &submit, VK_NULL_HANDLE) == VK_SUCCESS;
    if (submitted) {
        vkQueueWaitIdle(g_graphics_queue);
    }
    vkFreeCommandBuffers(g_device, g_command_pool, 1u, &command);
    vkDestroyBuffer(g_device, staging, nullptr);
    vkFreeMemory(g_device, staging_memory, nullptr);
    if (!submitted) {
        return false;
    }

    if (!existing) {
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = texture.image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.levelCount = texture.mip_count;
        view.subresourceRange.layerCount = 1u;
        if (vkCreateImageView(g_device, &view, nullptr, &texture.view) != VK_SUCCESS) {
            destroy_gpu_image(texture);
            return false;
        }
        const bool nearest = filter == PVR_FILTER_NEAREST;
        VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler.magFilter = nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
        sampler.minFilter = nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
        sampler.mipmapMode = filter == PVR_FILTER_TRILINEAR1 ||
                              filter == PVR_FILTER_TRILINEAR2
            ? VK_SAMPLER_MIPMAP_MODE_LINEAR
            : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.maxLod = static_cast<float>(texture.mip_count);
        if (vkCreateSampler(g_device, &sampler, nullptr, &texture.sampler) != VK_SUCCESS) {
            destroy_gpu_image(texture);
            return false;
        }
    }
    return true;
}

pc_endjinn_pvr::DecodedMip solid_mip(uint32_t rgba)
{
    pc_endjinn_pvr::DecodedMip mip{1u, 1u, std::vector<uint8_t>(4u)};
    std::memcpy(mip.pixels.data(), &rgba, 4u);
    return mip;
}

void write_texture_descriptor(const GpuTexture &texture)
{
    const GpuImage &color = texture.indexed ? g_white_texture : texture.image;
    const GpuImage &index = texture.indexed ? texture.image : g_default_index_texture;
    VkDescriptorImageInfo images[3] = {
        {color.sampler, color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {index.sampler, index.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {g_palette_texture.sampler, g_palette_texture.view,
         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    VkWriteDescriptorSet writes[3]{};
    for (uint32_t i = 0u; i < 3u; i++) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = texture.descriptor;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1u;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &images[i];
    }
    vkUpdateDescriptorSets(g_device, 3u, writes, 0u, nullptr);
}

bool allocate_texture_descriptor(GpuTexture &texture)
{
    VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc.descriptorPool = g_descriptor_pool;
    alloc.descriptorSetCount = 1u;
    alloc.pSetLayouts = &g_texture_set_layout;
    if (vkAllocateDescriptorSets(g_device, &alloc, &texture.descriptor) != VK_SUCCESS) {
        return false;
    }
    write_texture_descriptor(texture);
    return true;
}

bool create_builtin_textures()
{
    const std::vector<pc_endjinn_pvr::DecodedMip> white = {solid_mip(0xffffffffu)};
    pc_endjinn_pvr::DecodedMip index{1u, 1u, std::vector<uint8_t>(1u, 0u)};
    const auto palette = pc_endjinn_pvr::palette_rgba();
    pc_endjinn_pvr::DecodedMip palette_mip{
        1024u, 1u, std::vector<uint8_t>(palette.size() * sizeof(uint32_t))};
    std::memcpy(palette_mip.pixels.data(), palette.data(), palette_mip.pixels.size());
    g_uploaded_palette_revision = pc_endjinn_pvr::palette_revision();
    const bool uploaded =
           upload_gpu_image(g_white_texture, VK_FORMAT_R8G8B8A8_UNORM, white,
                            PVR_FILTER_NEAREST) &&
           upload_gpu_image(g_default_index_texture, VK_FORMAT_R8_UINT, {index},
                            PVR_FILTER_NEAREST) &&
           upload_gpu_image(g_palette_texture, VK_FORMAT_R8G8B8A8_UNORM,
                            {palette_mip}, PVR_FILTER_NEAREST);
    if (!uploaded) {
        return false;
    }
    GpuTexture untextured{};
    untextured.image = g_white_texture;
    if (!allocate_texture_descriptor(untextured)) {
        return false;
    }
    g_default_texture_descriptor = untextured.descriptor;
    return true;
}

bool update_palette_texture()
{
    const uint64_t revision = pc_endjinn_pvr::palette_revision();
    if (revision == g_uploaded_palette_revision) {
        return true;
    }
    const auto palette = pc_endjinn_pvr::palette_rgba();
    pc_endjinn_pvr::DecodedMip mip{
        1024u, 1u, std::vector<uint8_t>(palette.size() * sizeof(uint32_t))};
    std::memcpy(mip.pixels.data(), palette.data(), mip.pixels.size());
    if (!upload_gpu_image(g_palette_texture, VK_FORMAT_R8G8B8A8_UNORM, {mip},
                          PVR_FILTER_NEAREST)) {
        return false;
    }
    g_uploaded_palette_revision = revision;
    return true;
}

GpuTexture *gpu_texture_for(const DrawBatch &batch)
{
    if (!batch.textured) {
        return nullptr;
    }
    auto found = std::find_if(
        g_texture_cache.begin(), g_texture_cache.end(),
        [&](const GpuTexture &texture) {
            return texture.source == batch.texture &&
                   texture.source_format == batch.texture_format &&
                   texture.width == batch.texture_width &&
                   texture.height == batch.texture_height &&
                   texture.filter == batch.texture_filter;
        });
    const uint64_t revision = pc_endjinn_pvr::texture_revision(batch.texture);
    if (found != g_texture_cache.end() && found->revision == revision) {
        return &*found;
    }

    QueuedPrimitive primitive{};
    primitive.textured = true;
    primitive.texture = batch.texture;
    primitive.texture_format = batch.texture_format;
    primitive.texture_width = batch.texture_width;
    primitive.texture_height = batch.texture_height;
    primitive.texture_filter = batch.texture_filter;
    pc_endjinn_pvr::DecodedTexture decoded;
    if (!pc_endjinn_pvr::decode_texture(primitive, decoded)) {
        return nullptr;
    }
    if (found == g_texture_cache.end()) {
        g_texture_cache.push_back({});
        found = g_texture_cache.end() - 1;
        found->source = batch.texture;
        found->source_format = batch.texture_format;
        found->width = batch.texture_width;
        found->height = batch.texture_height;
        found->filter = batch.texture_filter;
    } else {
        destroy_gpu_image(found->image);
    }
    found->indexed = decoded.indexed;
    found->palette_base = decoded.palette_base;
    found->revision = revision;
    const VkFormat format = decoded.indexed ? VK_FORMAT_R8_UINT
                                            : VK_FORMAT_R8G8B8A8_UNORM;
    const pvr_filter_mode_t upload_filter =
        decoded.indexed ? PVR_FILTER_NEAREST : batch.texture_filter;
    if (!upload_gpu_image(found->image, format, decoded.mips, upload_filter)) {
        return nullptr;
    }
    if (found->descriptor == VK_NULL_HANDLE) {
        if (!allocate_texture_descriptor(*found)) {
            return nullptr;
        }
    } else {
        write_texture_descriptor(*found);
    }
    return &*found;
}

void destroy_frame_resources()
{
    destroy_texture_resources();
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
    if (g_opaque_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, g_opaque_pipeline, nullptr);
        g_opaque_pipeline = VK_NULL_HANDLE;
    }
    if (g_punch_through_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, g_punch_through_pipeline, nullptr);
        g_punch_through_pipeline = VK_NULL_HANDLE;
    }
    if (g_translucent_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, g_translucent_pipeline, nullptr);
        g_translucent_pipeline = VK_NULL_HANDLE;
    }
    if (g_modifier_volume_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, g_modifier_volume_pipeline, nullptr);
        g_modifier_volume_pipeline = VK_NULL_HANDLE;
    }
    if (g_modifier_exclude_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, g_modifier_exclude_pipeline, nullptr);
        g_modifier_exclude_pipeline = VK_NULL_HANDLE;
    }
    if (g_modifier_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, g_modifier_pipeline, nullptr);
        g_modifier_pipeline = VK_NULL_HANDLE;
    }
    if (g_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(g_device, g_pipeline_layout, nullptr);
        g_pipeline_layout = VK_NULL_HANDLE;
    }
    if (g_texture_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(g_device, g_texture_set_layout, nullptr);
        g_texture_set_layout = VK_NULL_HANDLE;
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
    constexpr VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    for (VkFormat candidate : candidates) {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(g_physical_device, candidate, &properties);
        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            g_depth_format = candidate;
            break;
        }
    }
    if (g_depth_format == VK_FORMAT_UNDEFINED) {
        return false;
    }
    VkImageCreateInfo image{};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.extent = {g_extent.width, g_extent.height, 1u};
    image.mipLevels = 1u;
    image.arrayLayers = 1u;
    image.format = g_depth_format;
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
    view.format = g_depth_format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
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
    depth.format = g_depth_format;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
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
    std::vector<char> punch_frag = read_shader_file("flat_punch.frag.spv");
    VkShaderModule vert_module = create_shader_module(vert);
    VkShaderModule frag_module = create_shader_module(frag);
    VkShaderModule punch_frag_module = create_shader_module(punch_frag);
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE ||
        punch_frag_module == VK_NULL_HANDLE) {
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
    VkVertexInputAttributeDescription attrs[3]{};
    attrs[0] = {0u, 0u, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PcVertex, position)};
    attrs[1] = {1u, 0u, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(PcVertex, color)};
    attrs[2] = {2u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof(PcVertex, uv)};
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1u;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = 3u;
    vertex_input.pVertexAttributeDescriptions = attrs;
    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    update_content_rect();
    VkViewport viewport{
        static_cast<float>(g_content_rect.offset.x),
        static_cast<float>(g_content_rect.offset.y),
        static_cast<float>(g_content_rect.extent.width),
        static_cast<float>(g_content_rect.extent.height),
        0.0f,
        1.0f};
    VkRect2D scissor = g_content_rect;
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1u;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1u;
    viewport_state.pScissors = &scissor;
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    // ponytail: 2D PVR packets mix cull directions; add per-context pipelines for 3D parity.
    raster.cullMode = VK_CULL_MODE_NONE;
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
    VkDescriptorSetLayoutBinding texture_bindings[3]{};
    for (uint32_t i = 0u; i < 3u; i++) {
        texture_bindings[i].binding = i;
        texture_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texture_bindings[i].descriptorCount = 1u;
        texture_bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo set_layout{};
    set_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_layout.bindingCount = 3u;
    set_layout.pBindings = texture_bindings;
    if (vkCreateDescriptorSetLayout(g_device, &set_layout, nullptr,
                                    &g_texture_set_layout) != VK_SUCCESS) {
        return false;
    }
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = 3072u;
    VkDescriptorPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.maxSets = 1024u;
    pool.poolSizeCount = 1u;
    pool.pPoolSizes = &pool_size;
    if (vkCreateDescriptorPool(g_device, &pool, nullptr, &g_descriptor_pool) != VK_SUCCESS) {
        return false;
    }
    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push.size = sizeof(TexturePush);
    VkPipelineLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout.setLayoutCount = 1u;
    layout.pSetLayouts = &g_texture_set_layout;
    layout.pushConstantRangeCount = 1u;
    layout.pPushConstantRanges = &push;
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
    bool ok = vkCreateGraphicsPipelines(
        g_device, VK_NULL_HANDLE, 1u, &pipe, nullptr, &g_opaque_pipeline) == VK_SUCCESS;

    stages[1].module = punch_frag_module;
    ok = ok && vkCreateGraphicsPipelines(
        g_device, VK_NULL_HANDLE, 1u, &pipe, nullptr, &g_punch_through_pipeline) == VK_SUCCESS;

    stages[1].module = frag_module;
    depth_state.depthWriteEnable = VK_FALSE;
    blend_att.blendEnable = VK_TRUE;
    blend_att.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_att.colorBlendOp = VK_BLEND_OP_ADD;
    blend_att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_att.alphaBlendOp = VK_BLEND_OP_ADD;
    ok = ok && vkCreateGraphicsPipelines(
        g_device, VK_NULL_HANDLE, 1u, &pipe, nullptr, &g_translucent_pipeline) == VK_SUCCESS;

    depth_state.depthTestEnable = VK_FALSE;
    depth_state.depthWriteEnable = VK_FALSE;
    depth_state.stencilTestEnable = VK_TRUE;
    depth_state.front.compareOp = VK_COMPARE_OP_ALWAYS;
    depth_state.front.passOp = VK_STENCIL_OP_REPLACE;
    depth_state.front.reference = 1u;
    depth_state.front.compareMask = 0xffu;
    depth_state.front.writeMask = 0xffu;
    depth_state.back = depth_state.front;
    blend_att.colorWriteMask = 0u;
    blend_att.blendEnable = VK_FALSE;
    ok = ok && vkCreateGraphicsPipelines(
        g_device, VK_NULL_HANDLE, 1u, &pipe, nullptr, &g_modifier_volume_pipeline) == VK_SUCCESS;

    depth_state.front.passOp = VK_STENCIL_OP_ZERO;
    depth_state.back = depth_state.front;
    ok = ok && vkCreateGraphicsPipelines(
        g_device, VK_NULL_HANDLE, 1u, &pipe, nullptr, &g_modifier_exclude_pipeline) == VK_SUCCESS;

    depth_state.front.compareOp = VK_COMPARE_OP_EQUAL;
    depth_state.front.passOp = VK_STENCIL_OP_KEEP;
    depth_state.back = depth_state.front;
    blend_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_att.blendEnable = VK_TRUE;
    blend_att.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ok = ok && vkCreateGraphicsPipelines(
        g_device, VK_NULL_HANDLE, 1u, &pipe, nullptr, &g_modifier_pipeline) == VK_SUCCESS;

    vkDestroyShaderModule(g_device, punch_frag_module, nullptr);
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
    const bool sync_created =
        vkCreateSemaphore(g_device, &sem, nullptr, &g_image_available) == VK_SUCCESS &&
        vkCreateSemaphore(g_device, &sem, nullptr, &g_render_finished) == VK_SUCCESS &&
        vkCreateFence(g_device, &fence, nullptr, &g_in_flight) == VK_SUCCESS;
    return sync_created && create_builtin_textures();
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

PcVertex make_vertex(float x, float y, float z, uint32_t argb, float u, float v)
{
    const float half_w = (float)g_vid_mode.width * (g_fsaa_enabled ? 1.0f : 0.5f);
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
    vertex.uv[0] = u;
    vertex.uv[1] = v;
    return vertex;
}

void emit_primitive(std::vector<PcVertex> &out, const QueuedPrimitive &p)
{
    const auto emit = [&](uint32_t i) {
        out.push_back(make_vertex(p.x[i], p.y[i], p.z[i], p.color[i],
                                  p.u[i], p.v[i]));
    };
    if (p.count == 3u) {
        emit(0u); emit(1u); emit(2u);
    } else if (p.count == 4u) {
        emit(0u); emit(1u); emit(2u);
        emit(0u); emit(2u); emit(3u);
    }
}

float primitive_average_z(const QueuedPrimitive &primitive)
{
    float z = 0.0f;
    for (uint32_t i = 0u; i < primitive.count; i++) {
        z += primitive.z[i];
    }
    return primitive.count > 0u ? z / static_cast<float>(primitive.count) : 0.0f;
}

FrameDrawData build_frame_draw_data()
{
    const std::vector<QueuedPrimitive> &queued = pc_endjinn_pvr::primitives();
    FrameDrawData frame;
    frame.vertices.reserve(queued.size() * 6u);

    const auto append_list = [&](pvr_list_t list, bool sort_back_to_front,
                                 bool modifier_volume) {
        std::vector<const QueuedPrimitive *> primitives;
        primitives.reserve(queued.size());
        for (const QueuedPrimitive &primitive : queued) {
            if (primitive.list == list &&
                primitive.modifier_volume == modifier_volume) {
                primitives.push_back(&primitive);
            }
        }
        if (sort_back_to_front) {
            std::stable_sort(
                primitives.begin(),
                primitives.end(),
                [](const QueuedPrimitive *a, const QueuedPrimitive *b) {
                    // PVR screen-space Z is inverse depth: smaller values are farther away.
                    return primitive_average_z(*a) < primitive_average_z(*b);
                });
        }
        for (const QueuedPrimitive *primitive : primitives) {
            const bool same_batch = !frame.batches.empty() &&
                frame.batches.back().list == list &&
                frame.batches.back().textured == primitive->textured &&
                frame.batches.back().texture == primitive->texture &&
                frame.batches.back().texture_format == primitive->texture_format &&
                frame.batches.back().texture_width == primitive->texture_width &&
                frame.batches.back().texture_height == primitive->texture_height &&
                frame.batches.back().texture_filter == primitive->texture_filter &&
                frame.batches.back().modifier == primitive->modifier &&
                frame.batches.back().modifier_volume == primitive->modifier_volume &&
                frame.batches.back().modifier_mode == primitive->modifier_mode;
            if (!same_batch) {
                DrawBatch batch{};
                batch.list = list;
                batch.first_vertex = static_cast<uint32_t>(frame.vertices.size());
                batch.textured = primitive->textured;
                batch.texture = primitive->texture;
                batch.texture_format = primitive->texture_format;
                batch.texture_width = primitive->texture_width;
                batch.texture_height = primitive->texture_height;
                batch.texture_filter = primitive->texture_filter;
                batch.modifier = primitive->modifier;
                batch.modifier_volume = primitive->modifier_volume;
                batch.modifier_mode = primitive->modifier_mode;
                const uint32_t pixel_format = (primitive->texture_format >> 27u) & 7u;
                batch.palette_base = pixel_format == PVR_PIXEL_MODE_PAL_4BPP
                    ? ((primitive->texture_format >> 21u) & 0x3fu) * 16u
                    : ((primitive->texture_format >> 25u) & 0x03u) * 256u;
                frame.batches.push_back(batch);
            }
            const uint32_t before = static_cast<uint32_t>(frame.vertices.size());
            emit_primitive(frame.vertices, *primitive);
            frame.batches.back().vertex_count +=
                static_cast<uint32_t>(frame.vertices.size()) - before;
        }
    };

    append_list(PVR_LIST_OP_MOD, false, true);
    append_list(PVR_LIST_TR_MOD, false, true);
    append_list(PVR_LIST_OP_POLY, false, false);
    append_list(PVR_LIST_PT_POLY, false, false);
    append_list(PVR_LIST_TR_POLY, g_translucent_autosort, false);
    return frame;
}

bool draw_frame(const FrameDrawData &frame)
{
    if (g_device == VK_NULL_HANDLE || g_swapchain == VK_NULL_HANDLE ||
        g_opaque_pipeline == VK_NULL_HANDLE ||
        g_punch_through_pipeline == VK_NULL_HANDLE ||
        g_translucent_pipeline == VK_NULL_HANDLE ||
        g_modifier_volume_pipeline == VK_NULL_HANDLE ||
        g_modifier_exclude_pipeline == VK_NULL_HANDLE ||
        g_modifier_pipeline == VK_NULL_HANDLE || g_command_buffers.empty()) {
        set_window_title("pc-enDjinn - draw unavailable");
        return false;
    }
    if (!update_palette_texture()) {
        set_window_title("pc-enDjinn - palette upload failed");
        return false;
    }
    for (const DrawBatch &batch : frame.batches) {
        if (batch.textured && gpu_texture_for(batch) == nullptr) {
            std::fprintf(stderr, "pc-enDjinn: texture decode/upload failed\n");
        }
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

    if (!ensure_vertex_buffer(frame.vertices.size())) {
        set_window_title("pc-enDjinn - vertex buffer failed");
        return false;
    }
    if (!frame.vertices.empty()) {
        void *mapped = nullptr;
        const VkDeviceSize bytes =
            static_cast<VkDeviceSize>(frame.vertices.size() * sizeof(PcVertex));
        if (vkMapMemory(g_device, g_vertex_memory, 0u, bytes, 0u, &mapped) != VK_SUCCESS) {
            set_window_title("pc-enDjinn - vertex map failed");
            return false;
        }
        std::memcpy(mapped, frame.vertices.data(), static_cast<size_t>(bytes));
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
    clears[0].color.float32[0] = 0.0f;
    clears[0].color.float32[1] = 0.0f;
    clears[0].color.float32[2] = 0.0f;
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
    VkClearAttachment content_clear{};
    content_clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    content_clear.colorAttachment = 0u;
    content_clear.clearValue.color.float32[0] = g_bg_color[0];
    content_clear.clearValue.color.float32[1] = g_bg_color[1];
    content_clear.clearValue.color.float32[2] = g_bg_color[2];
    content_clear.clearValue.color.float32[3] = 1.0f;
    VkClearRect content_clear_rect{};
    content_clear_rect.rect = g_content_rect;
    content_clear_rect.layerCount = 1u;
    vkCmdClearAttachments(cmd, 1u, &content_clear, 1u, &content_clear_rect);
    if (!frame.vertices.empty()) {
        VkDeviceSize offset = 0u;
        vkCmdBindVertexBuffers(cmd, 0u, 1u, &g_vertex_buffer, &offset);
        for (const DrawBatch &batch : frame.batches) {
            if (batch.vertex_count == 0u) {
                continue;
            }
            VkPipeline pipeline = g_opaque_pipeline;
            if (batch.modifier_volume) {
                // ponytail: this is a 2D stencil mask; add PVR 3D winding only
                // when a project needs closed OTHER_POLY shadow volumes.
                pipeline = batch.modifier_mode == PVR_MODIFIER_EXCLUDE_LAST_POLY
                    ? g_modifier_exclude_pipeline : g_modifier_volume_pipeline;
            } else if (batch.modifier) {
                pipeline = g_modifier_pipeline;
            } else if (batch.list == PVR_LIST_PT_POLY) {
                pipeline = g_punch_through_pipeline;
            } else if (batch.list == PVR_LIST_TR_POLY) {
                pipeline = g_translucent_pipeline;
            }
            GpuTexture *texture = gpu_texture_for(batch);
            const VkDescriptorSet descriptor = texture == nullptr
                ? g_default_texture_descriptor
                : texture->descriptor;
            TexturePush push{};
            push.indexed = texture != nullptr && texture->indexed ? 1u : 0u;
            push.palette_base = texture != nullptr
                ? texture->palette_base
                : batch.palette_base;
            push.filter_mode = static_cast<uint32_t>(batch.texture_filter);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    g_pipeline_layout, 0u, 1u, &descriptor,
                                    0u, nullptr);
            vkCmdPushConstants(cmd, g_pipeline_layout,
                               VK_SHADER_STAGE_FRAGMENT_BIT, 0u,
                               sizeof(push), &push);
            vkCmdDraw(cmd, batch.vertex_count, 1u, batch.first_vertex, 0u);
        }
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
        g_last_window_width,
        g_last_window_height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (g_window == nullptr) {
        std::fprintf(stderr, "pc-enDjinn: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_AddEventWatch(pc_endjinn_event_watch, nullptr);
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

namespace pc_endjinn {

vid_mode_t *video_mode() { return &g_vid_mode; }

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
    g_vid_mode.width = 640;
    g_vid_mode.height = 480;
}

void pvr_init(const pvr_init_params_t *params)
{
    g_translucent_autosort = params == nullptr || params->autosort_disabled == 0u;
    g_fsaa_enabled = params != nullptr && params->fsaa_enabled != 0;
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
        SDL_DelEventWatch(pc_endjinn_event_watch, nullptr);
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    pc_endjinn_input_shutdown();
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
    pc_endjinn_pvr::scene_begin();
}

void pvr_scene_finish(void)
{
    update_window_mode();
    if (g_device == VK_NULL_HANDLE) {
        return;
    }
    const FrameDrawData frame = build_frame_draw_data();
    (void)draw_frame(frame);
    g_presented_frames++;
    if (g_window != nullptr && (g_presented_frames <= 3u || (g_presented_frames % 30u) == 0u)) {
        char title[128];
        std::snprintf(
            title,
            sizeof(title),
            "pc-enDjinn - pvr primitives %zu - vk verts %zu",
            pc_endjinn_pvr::primitives().size(),
            frame.vertices.size());
        SDL_SetWindowTitle(g_window, title);
    }
}

void pvr_list_begin(pvr_list_t list) { pc_endjinn_pvr::list_begin(list); }
void pvr_list_finish(void) {}
void pvr_wait_render_done(void) {}
void pvr_set_pal_format(pvr_palfmt_t mode) {
    pc_endjinn_pvr::palette_format(mode);
}

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
    return pc_endjinn_pvr::dr_target();
}

void pvr_dr_commit(void *ptr)
{
    pc_endjinn_pvr::dr_commit(ptr);
}

}  // namespace pc_endjinn
