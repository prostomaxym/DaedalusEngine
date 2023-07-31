#version 450 core

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
}vs_out;

struct Scene
{
    mat4 projection_view;
    mat4 model;
    vec3 view_pos;
};

uniform Scene u_scene;

void main()
{
    vs_out.frag_pos = vec3((u_scene.model * vec4(vin_vertices, 1.0)).xyz);
    vs_out.uv = vin_texcoord;
    vs_out.normals = mat3(transpose(inverse(u_scene.model))) * vin_normals;

    gl_Position = u_scene.projection_view * vec4(vs_out.frag_pos, 1.0);
}