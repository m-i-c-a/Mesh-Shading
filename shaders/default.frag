#version 450

// layout(set=0, binding=1) uniform sampler2D textures[256];

// layout(push_constant) uniform PushConsts {
//     int draw_idx;
//     int tex_idx;
// } push_consts;

layout(location=0) in vec3 in_worldPos;
layout(location=1) in vec3 in_normal;
layout(location=2) in vec3 in_viewPos;

layout(set=0, binding=1) uniform LightUBO
{
    vec4 pos;
    vec4 intensity;
} lightUBO;

layout(set=1, binding=0) uniform PerMatUBO
{
    vec4  albedo;
    // float metallic;
    // float roughness;
    // float ao;
} matUBO;

layout(location=0) out vec4 out_color;

vec3 lambertianBRDF(in vec3 albedo)
{
    return albedo / 3.1415;
}

void main()
{
    vec3 normal = normalize(in_normal);

    vec3 outgoingLight = lambertianBRDF(matUBO.albedo.rgb) * lightUBO.intensity.rgb * dot(normal, normalize(in_worldPos - lightUBO.pos.xyz));

    out_color = vec4(outgoingLight, 1.0f);
}