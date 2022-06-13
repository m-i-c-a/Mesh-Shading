#version 450

// layout(set=0, binding=1) uniform sampler2D textures[256];

// layout(push_constant) uniform PushConsts {
//     int draw_idx;
//     int tex_idx;
// } push_consts;

layout(location=0) in vec2 in_uv;

layout(location=0) out vec4 out_color;

void main()
{
    out_color = vec4(0.17f, 0.68f, 0.62f, 1.0f);
    // out_color = texture(textures[push_consts.tex_idx], in_uv);
}