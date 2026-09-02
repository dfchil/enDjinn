#version 440

layout(location = 0) in vec4 frag_color;
layout(location = 1) in vec2 frag_uv;
layout(location = 2) in vec3 frag_position;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D color_texture;
layout(set = 0, binding = 1) uniform usampler2D index_texture;
layout(set = 0, binding = 2) uniform sampler2D palette_texture;

struct ModifierTriangle {
    vec4 a;
    vec4 b;
    vec4 c;
    uvec4 state;
};

layout(std430, set = 0, binding = 3) readonly buffer ModifierEvents {
    ModifierTriangle triangles[];
} modifiers;

layout(push_constant) uniform TextureState {
    uint indexed;
    uint palette_base;
    uint filter_mode;
    uint unused;
    uint modifier_count;
    uint modifier_area;
    uint reserved0;
    uint reserved1;
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

float cross2(vec2 a, vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

bool top_left_edge(vec2 edge)
{
    /* Half-open ownership prevents a shared mesh edge from contributing two
     * XOR crossings at the same sample. NDC Y follows the submitted PVR Y. */
    return edge.y < 0.0 || (edge.y == 0.0 && edge.x > 0.0);
}

bool triangle_crosses_receiver(ModifierTriangle triangle)
{
    vec2 a = triangle.a.xy;
    vec2 b = triangle.b.xy;
    vec2 c = triangle.c.xy;
    float az = triangle.a.z;
    float bz = triangle.b.z;
    float cz = triangle.c.z;
    vec2 p = frag_position.xy;
    float denominator = cross2(b - a, c - a);
    if (abs(denominator) < 0.0000001) {
        return false;
    }
    if (denominator < 0.0) {
        vec2 swap_position = b;
        b = c;
        c = swap_position;
        float swap_depth = bz;
        bz = cz;
        cz = swap_depth;
        denominator = -denominator;
    }
    vec2 edges[3] = vec2[3](b - a, c - b, a - c);
    float coverage[3] = float[3](cross2(edges[0], p - a),
                                 cross2(edges[1], p - b),
                                 cross2(edges[2], p - c));
    const float edge_epsilon = 0.0000001;
    for (int edge = 0; edge < 3; edge++) {
        if (coverage[edge] < -edge_epsilon ||
            (abs(coverage[edge]) <= edge_epsilon &&
             !top_left_edge(edges[edge]))) {
            return false;
        }
    }
    float wa = cross2(b - p, c - p) / denominator;
    float wb = cross2(c - p, a - p) / denominator;
    float wc = 1.0 - wa - wb;
    float modifier_depth = wa * az + wb * bz + wc * cz;
    return modifier_depth > frag_position.z;
}

bool area_one_at_receiver()
{
    bool current = false;
    bool summary = false;
    bool pending = false;
    for (uint i = 0u; i < state.modifier_count; i++) {
        ModifierTriangle triangle = modifiers.triangles[i];
        if (triangle_crosses_receiver(triangle)) {
            current = triangle.state.y != 0u ? true : !current;
        }
        pending = true;
        if (triangle.state.x != 0u) {
            summary = triangle.state.z == 2u ? summary && !current
                                             : summary || current;
            current = false;
            pending = false;
        }
    }
    /* Raw open/planar OR streams do not necessarily carry VolumeLast. */
    if (pending) {
        summary = summary || current;
    }
    return summary;
}

void main()
{
    bool area_one = area_one_at_receiver();
    if ((state.modifier_area == 1u && area_one) ||
        (state.modifier_area == 2u && !area_one)) {
        discard;
    }
    out_color = texture_color() * frag_color;
}
