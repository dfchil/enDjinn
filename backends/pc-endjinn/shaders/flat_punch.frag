#version 440

layout(location = 0) in vec4 frag_color;
layout(location = 1) in vec2 frag_uv;
layout(location = 3) in vec4 frag_offset;
layout(location = 0) out vec4 out_color;
layout(set = 0, binding = 0) uniform sampler2D color_texture;
layout(set = 0, binding = 1) uniform usampler2D index_texture;
layout(set = 0, binding = 2) uniform sampler2D palette_texture;

layout(push_constant) uniform TextureState {
    uint indexed;
    uint palette_base;
    uint filter_mode;
    uint texture_mode; /* env in bits 0-1, UV clamp 2-3, UV flip 4-5 (U = 2, V = 1) */
    uint modifier_count;
    uint modifier_area;
    uint reserved0;
    uint reserved1;
} state;

int wrap_coord(int p, int size, bool clamped, bool flipped)
{
    if (clamped) {
        return clamp(p, 0, size - 1);
    }
    if (flipped) {
        int m = (p % (2 * size) + 2 * size) % (2 * size);
        return m < size ? m : 2 * size - 1 - m;
    }
    return (p % size + size) % size;
}

vec4 palette_color(ivec2 pixel)
{
    ivec2 size = textureSize(index_texture, 0);
    pixel = ivec2(wrap_coord(pixel.x, size.x, (state.texture_mode & 8u) != 0u,
                             (state.texture_mode & 32u) != 0u),
                  wrap_coord(pixel.y, size.y, (state.texture_mode & 4u) != 0u,
                             (state.texture_mode & 16u) != 0u));
    uint index = texelFetch(index_texture, pixel, 0).r;
    return texelFetch(palette_texture,
                      ivec2(int(state.palette_base + index), 0), 0);
}

// The PVR's texture environments (pvr_txr_shading_mode_t).
vec4 combine(vec4 texel, vec4 color)
{
    uint env = state.texture_mode & 3u;
    if (env == 0u) {
        return texel; /* REPLACE */
    }
    if (env == 1u) {
        return vec4(texel.rgb * color.rgb, texel.a); /* MODULATE */
    }
    if (env == 2u) {
        return vec4(mix(color.rgb, texel.rgb, texel.a), color.a); /* DECAL */
    }
    return texel * color; /* MODULATEALPHA */
}

// The offset colour (specular, bit 6 of texture_mode), added after the environment.
vec4 add_offset(vec4 color)
{
    if ((state.texture_mode & 64u) != 0u) {
        color.rgb = min(color.rgb + frag_offset.rgb, vec3(1.0));
    }
    return color;
}

vec4 texture_color()
{
    if (state.indexed == 0u) {
        return texture(color_texture, frag_uv);
    }
    vec2 position = frag_uv * vec2(textureSize(index_texture, 0)) - 0.5;
    ivec2 pixel = ivec2(floor(position));
    if (state.filter_mode == 0u) {
        return palette_color(ivec2(floor(position + 0.5)));
    }
    vec2 weight = fract(position);
    return mix(mix(palette_color(pixel), palette_color(pixel + ivec2(1, 0)), weight.x),
               mix(palette_color(pixel + ivec2(0, 1)),
                   palette_color(pixel + ivec2(1, 1)), weight.x), weight.y);
}

void main()
{
    vec4 color = add_offset(combine(texture_color(), frag_color));
    if (color.a < 0.5) {
        discard;
    }
    out_color = color;
}
