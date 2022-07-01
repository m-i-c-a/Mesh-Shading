#version 450

vec3 lambertianBRDF(in vec3 albedo)
{
    return albedo / 3.1415;
}

/* 
 * Position-invariant BRDF.
 *
 */
void BRDF(in vec3 incomingDir, in vec3 outgoingDir, in vec3 incomingLight, in vec3 normal)
{

}