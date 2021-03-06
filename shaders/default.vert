#version 450

layout(location=0) in vec3 a_pos;
layout(location=1) in vec2 a_uv;
layout(location=2) in vec3 a_norm;

layout(set=0, binding=0) uniform PerFrameUBO
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3 viewPos;
} FrameUBO;

layout(location=0) out vec3 out_worldPos;
layout(location=1) out vec3 out_normal;
layout(location=2) out vec3 out_viewPos;

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

// struct Vertex
// {
//     float vx, vy, vz;
//     float ux, uy;
//     float nx, ny, nz;
// };

// layout(set=0, binding=0) readonly buffer VertexBuffer
// {
//     Vertex vertices[];
// };

void main()
{
    // Vertex vertexInfo = vertices[gl_VertexIndex];
    // gl_Position = vec4(vertexInfo.vx, vertexInfo.vy, vertexInfo.vz, 1.0f); 
    // out_norm = vec3(vertexInfo.nx, vertexInfo.ny, vertexInfo.nz);

    gl_Position = FrameUBO.projMatrix * FrameUBO.viewMatrix * vec4(a_pos, 1.0f);

    out_worldPos = a_pos;
    out_normal   = a_norm;
    out_viewPos  = FrameUBO.viewPos;
}