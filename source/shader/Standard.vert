#version 450 core

layout (location = 0) in vec3 vin_vertices;
layout (location = 1) in vec2 vin_texcoord;
layout (location = 2) in vec3 vin_normals;
layout (location = 3) in vec3 vin_tangent;
layout (location = 4) in vec3 vin_bitangent;

out VS_OUT
{
    out vec3 frag_pos;
    out vec4 frag_pos_light_space;
    out vec2 uv;
    out vec3 normals;
    out mat3 TBN;
}vs_out;

struct ObjectData
{
    sampler2D tex_diffuse;
    sampler2D tex_specular;
    sampler2D tex_normal;
    mat4 model_mat;
    vec3 k_ambient;
    vec3 k_diffuse;
    vec3 k_specular;
    float shininess;
    int enable_diffuse_map;
    int enable_specular_map;
    int enable_normal_map;
};

struct Scene
{
    mat4 projection_view;
    vec3 view_pos;
};

layout (std140, binding = 0) uniform SceneUBO
{
    Scene ubo_scene;
};

uniform ObjectData u_object; 
uniform mat4 u_light_space_matrix;

void main()
{
    vs_out.frag_pos = vec3((u_object.model_mat * vec4(vin_vertices, 1.0)).xyz);
    vs_out.frag_pos_light_space = u_light_space_matrix * vec4(vs_out.frag_pos, 1.0);
    vs_out.uv = vin_texcoord;
    mat3 normal_matrix = transpose(inverse(mat3(u_object.model_mat)));
    vs_out.normals = normalize(normal_matrix * vin_normals);

    if(u_object.enable_normal_map == 1)
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