#version 450

layout(location=0) in vec3 a_pos;
layout(location=1) in vec2 a_uv;

layout(location=0) out vec2 out_uv;

struct Material
{
    int tex_idx;
};

struct PerObjData
{
    vec3 translation;
};

// Whatever passes cull has its material in here
// First N materials may be reserved for common materials. 
// These common materials are rendered frequently enough
// that we just always keep them present
// bush, water, and tree material for ex
layout(set=0, binding=0) buffer ssbo_material_buffer
{
    Material ssbo_materials[];
};

// Whatever passes cull has an index in here
layout(set=0, binding=1) buffer ssbo_draw_buffer
{
    PerObjData ssbo_draws[];
};

layout(push_constant) uniform PushConsts
{
    int mat_idx;
    int draw_idx;
} push_consts;

void main()
{
}