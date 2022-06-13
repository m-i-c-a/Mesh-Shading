#version 450

// layout(location=0) in vec3 a_pos;
// layout(location=1) in vec2 a_uv;

layout(location=0) out vec2 out_uv;

// struct PerObjData
// {
//     vec4 translation;
// };

// layout(set=0, binding=0) buffer ssbo_draw_buffer
// {
//     PerObjData obj_data[];
// } draw_ssbo;

// layout(push_constant) uniform PushConsts
// {
//     int draw_idx;
//     int tex_idx;
// } push_consts;

struct Vertex
{
    float vx, vy, vz;
    float ux, uy;
};

layout(set=0, binding=0) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

void main()
{
    Vertex vertexInfo = vertices[gl_VertexIndex];
    gl_Position = vec4(vertexInfo.vx, vertexInfo.vy, vertexInfo.vz, 1.0f); 
    out_uv = vec2(vertexInfo.ux, vertexInfo.uy);
}