#vertex
#version 450 core
#include "Scene.hglsl"

layout (location = 0) in vec3 vin_vertices;
layout (location = 1) in vec2 vin_texcoord;
layout (location = 2) in vec3 vin_normals;
layout (location = 3) in vec3 vin_tangent;
layout (location = 4) in vec3 vin_bitangent;

out VS_OUT
{
    out vec3 frag_pos;
    out vec2 uv;
    out vec3 normals;
    out mat3 TBN;
}vs_out;

struct Material
{
    sampler2D albedo;
    sampler2D normal;
    sampler2D metallic;
    sampler2D roughness;
    sampler2D ao;
};

uniform Material u_material;
uniform mat4 u_model_mat;

void main()
{
    vs_out.frag_pos = vec3((u_model_mat * vec4(vin_vertices, 1.0)).xyz);
    vs_out.uv = vin_texcoord;
    mat3 normal_matrix = transpose(inverse(mat3(u_model_mat)));
    vs_out.normals = normalize(normal_matrix * vin_normals);

    vs_out.TBN = mat3
    (
        normalize(vec3(u_model_mat * vec4(vin_tangent, 0.0))),
        normalize(vec3(u_model_mat * vec4(vin_bitangent, 0.0))),
        normalize(vec3(u_model_mat * vec4(vin_normals, 0.0)))
    );

    gl_Position = ubo_scene.projection_view * vec4(vs_out.frag_pos, 1.0);
}


// ----------------------------------------------------------------------------- //
#fragment
#version 450 core

// -------------------------------------------Inputs/Outputs------------------------------------------------- //
in VS_OUT
{
    in vec3 frag_pos;
    in vec2 uv;
    in vec3 normals;
    in mat3 TBN;
}fs_in;

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gMetallicRoughnessAO; // R: Metallic, G: Roughness, B: AO

// ----------------------------------------------Structures-------------------------------------------------- //
struct Material
{
    sampler2D albedo;
    sampler2D normal;
    sampler2D metallic;
    sampler2D roughness;
    sampler2D ao;
};

uniform Material u_material;

void main()
{
    const float alpha = texture(u_material.albedo, fs_in.uv).a;
    if (alpha < 0.99)
        discard;

    vec3 normal = texture(u_material.normal, fs_in.uv).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(fs_in.TBN * normal);

    gPosition = vec4(fs_in.frag_pos, 1.0);
    gNormal = vec4(normal, 1.0);
    gAlbedo = vec4(texture(u_material.albedo, fs_in.uv).rgb, 1.0);
    gMetallicRoughnessAO.r = texture(u_material.metallic, fs_in.uv).r;
    gMetallicRoughnessAO.g = texture(u_material.roughness, fs_in.uv).r;
    gMetallicRoughnessAO.b = texture(u_material.ao, fs_in.uv).r;
    gMetallicRoughnessAO.a = 1.0;
}