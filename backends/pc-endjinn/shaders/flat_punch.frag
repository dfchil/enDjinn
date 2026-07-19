#version 440

layout(location = 0) in vec4 frag_color;
layout(location = 1) in vec2 frag_uv;
layout(location = 0) out vec4 out_color;
layout(set = 0, binding = 0) uniform sampler2D color_texture;
layout(set = 0, binding = 1) uniform usampler2D index_texture;
layout(set = 0, binding = 2) uniform sampler2D palette_texture;

layout(push_constant) uniform TextureState {
    uint indexed;
    uint palette_base;
    uint filter_mode;
    uint unused;
} state;

vec4 palette_color(ivec2 pixel)
{
    ivec2 size = textureSize(index_texture, 0);
    pixel = ivec2((pixel.x % size.x + size.x) % size.x,
                  (pixel.y % size.y + size.y) % size.y);
    uint index = texelFetch(index_texture, pixel, 0).r;
    return texelFetch(palette_texture,
                      ivec2(int(state.palette_base + index), 0), 0);
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
    vec4 color = texture_color() * frag_color;
    if (color.a < 0.5) {
        discard;
    }
    out_color = color;
}
