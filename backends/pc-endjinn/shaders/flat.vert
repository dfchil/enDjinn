#version 440

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_offset;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 frag_uv;
layout(location = 2) out vec4 frag_position;
layout(location = 3) out vec4 frag_offset;

void main()
{
    // in_position.w is the vertex's 1/w. A clip-space w of its inverse leaves
    // the position and depth as they were but makes UVs and colours
    // perspective-correct; with w = 1 they were affine and textures swam.
    float w = in_position.w > 0.0 ? 1.0 / in_position.w : 1.0;
    gl_Position = vec4(in_position.xyz * w, w);
    frag_color = in_color;
    frag_uv = in_uv;
    frag_offset = in_offset;
    // Modifier volumes need the position linear on screen: (p * w, w) divides
    // back to exactly that per fragment.
    frag_position = vec4(in_position.xyz * w, w);
}
