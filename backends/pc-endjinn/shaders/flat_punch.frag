#version 440

layout(location = 0) in vec4 frag_color;
layout(location = 0) out vec4 out_color;

void main()
{
    if (frag_color.a < 0.5) {
        discard;
    }
    out_color = frag_color;
}
