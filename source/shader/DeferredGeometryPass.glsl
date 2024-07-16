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

struct ObjectData
{
    sampler2D tex_diffuse;
    sampler2D tex_specular;
    sampler2D tex_normal;
    sampler2D tex_height;
    mat4 model_mat;
    vec3 k_ambient;
    vec3 k_diffuse;
    vec3 k_specular;
    float shininess;
    bool enable_diffuse_map;
    bool enable_specular_map;
    bool enable_normal_map;
    bool enable_height_map;
};

uniform ObjectData u_object;

void main()
{
    vs_out.frag_pos = vec3((u_object.model_mat * vec4(vin_vertices, 1.0)).xyz);
    vs_out.uv = vin_texcoord;
    mat3 normal_matrix = transpose(inverse(mat3(u_object.model_mat)));
    vs_out.normals = normalize(normal_matrix * vin_normals);

    if (u_object.enable_normal_map)
    {
        vs_out.TBN = mat3
        (
            normalize(vec3(u_object.model_mat * vec4(vin_tangent, 0.0))),
            normalize(vec3(u_object.model_mat * vec4(vin_bitangent, 0.0))),
            normalize(vec3(u_object.model_mat * vec4(vin_normals, 0.0)))
        );
    }
    else
    {
        vs_out.TBN = mat3(vec3(0.0,0.0,0.0), vec3(0.0,0.0,0.0), vec3(0.0,0.0,0.0));
    }

    gl_Position = ubo_scene.projection_view * vec4(vs_out.frag_pos, 1.0);
}


// ----------------------------------------------------------------------------- //
#fragment
#version 450 core
#include "Scene.hglsl"

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
layout (location = 2) out vec4 gAmbient;
layout (location = 3) out vec4 gSpec;
layout (location = 4) out vec4 gAlbedo;
layout (location = 5) out float gShininess;

// ----------------------------------------------Structures-------------------------------------------------- //
struct ObjectData
{
    sampler2D tex_diffuse;
    sampler2D tex_specular;
    sampler2D tex_normal;
    sampler2D tex_height;
    mat4 model_mat;
    vec3 k_ambient;
    vec3 k_diffuse;
    vec3 k_specular;
    float shininess;
    bool enable_diffuse_map;
    bool enable_specular_map;
    bool enable_normal_map;
    bool enable_height_map;
};

uniform ObjectData u_object;

vec2 ParallaxMapping(vec2 tex_coords, vec3 view_dir)
{
    const float heightScale = 0.1f;
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), view_dir)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = view_dir.xy / view_dir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords     = tex_coords;
    float currentDepthMapValue = texture(u_object.tex_height, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(u_object.tex_height, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(u_object.tex_height, prevTexCoords).r - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

void main()
{
    const vec3 view_dir = normalize(ubo_scene.view_pos - fs_in.frag_pos);
    const vec2 tex_coord = u_object.enable_height_map ? ParallaxMapping(fs_in.uv, view_dir) : fs_in.uv;

    // TODO: implement better way to show transparent objects in front of opaque objects
    const float alpha = u_object.enable_diffuse_map ? texture(u_object.tex_diffuse, tex_coord).a : 1.0;
    if (alpha < 0.99)
        discard;

    vec3 diffuse = u_object.enable_diffuse_map ? texture(u_object.tex_diffuse, tex_coord).rgb : vec3(1.0);
    diffuse *= u_object.k_diffuse;
    vec3 spec = u_object.enable_specular_map ? texture(u_object.tex_specular, tex_coord).rgb : vec3(1.0);
    spec *= u_object.k_specular;

    vec3 normal = vec3(0.0);
    if (u_object.enable_normal_map)
    {
        normal = texture(u_object.tex_normal, tex_coord).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        normal = normalize(fs_in.TBN * normal);
    }
    else
    {
        normal = fs_in.normals;
    }

    gPosition = vec4(fs_in.frag_pos, 1.0);
    gNormal = vec4(normal, 1.0);
    gAmbient = vec4(u_object.k_ambient * diffuse, 1.0);
    gSpec = vec4(spec, 1.0);
    gAlbedo = vec4(diffuse, 1.0);
    gShininess = 1.0 / u_object.shininess;
}