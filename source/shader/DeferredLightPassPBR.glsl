#vertex
#version 450 core

layout (location = 0) in vec3 vin_vertices;
layout (location = 1) in vec2 vin_texcoord;

out vec2 vout_uv;

void main()
{
    vout_uv = vin_texcoord;
    gl_Position = vec4(vin_vertices, 1.0);
}


// ----------------------------------------------------------------------------- //
#fragment
#version 450 core
#include "Scene.hglsl"
#include "LightSSBO.hglsl"

// -------------------------------------------Inputs/Outputs------------------------------------------------- //

in vec2 vout_uv;
out vec4 fout_color;

uniform sampler2D u_gPosition;
uniform sampler2D u_gNormal;
uniform sampler2D u_gAlbedo;
uniform sampler2D u_gMetallicRoughnessAO;

// ------------------------------------------------- General Functions ------------------------------------------------ //
const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 frag_pos = texture(u_gPosition, vout_uv).rgb;
    vec3 N = texture(u_gNormal, vout_uv).rgb;
    vec3 albedo = texture(u_gAlbedo, vout_uv).rgb;
    float metallic = texture(u_gMetallicRoughnessAO, vout_uv).r;
    float roughness = texture(u_gMetallicRoughnessAO, vout_uv).g;
    float ao = texture(u_gMetallicRoughnessAO, vout_uv).b;

    vec3 V = normalize(ubo_scene.view_pos - frag_pos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < ssbo_lights.length(); ++i)
    {
        vec3 L = normalize(ssbo_lights[i].position - frag_pos);
        vec3 H = normalize(V + L);
        //float distance = length(lightPositions[i] - WorldPos);
        //float attenuation = 1.0 / (distance * distance);
        vec3 radiance = ssbo_lights[i].diffuse; //* attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    fout_color = vec4(color, 1.0);
}