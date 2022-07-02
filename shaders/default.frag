#version 450

// #define PI 3.1415
// const float PI = 3.1415;

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
    float roughness;
    // float metallic;
    // float ao;
} matUBO;

layout(location=0) out vec4 out_color;

vec3 lambertianBRDF(in vec3 albedo)
{
    return albedo / 3.1415;
}

// vec3 NDF_GGX(in vec3 vH, in float roughness)
// {
//     float r2 = roughness * roughness;
//     float f = (vH * r2 - vH) * vH + 1.0;
//     return (r2 / 3.1415 * f * f);
// }

vec3 cookTorrenceBDRF(in vec3 vLight, in vec3 vView,  in vec3 vNorm)
{
    vec3 vHalf = normalize(vNorm + vLight);

    vec3 diffuse = lambertianBRDF(matUBO.albedo.xyz);
    // vec3 specular = NDF_GGX(vHalf, matUBO.roughness) / (4.0 * dot(vNorm, vView) * dot(vNorm, vLight));

    return diffuse; // + specular;
}

void main()
{
    vec3 vNorm = normalize(in_normal);
    vec3 vLight = normalize(lightUBO.pos.xyz - in_worldPos);
    vec3 vView = normalize(in_viewPos - in_worldPos);

    vec3 outgoingLight = cookTorrenceBDRF(vLight, vView, vNorm) * lightUBO.intensity.rgb * dot(vNorm, vLight);

    out_color = vec4(outgoingLight, 1.0f);
}