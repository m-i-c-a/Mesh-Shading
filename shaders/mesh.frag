#version 450

layout(location=0) in vec3 in_norm;

layout(location=0) out vec4 out_color;

void main()
{
    out_color = vec4(0.17f, 0.68f, 0.62f, 1.0f);
}