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
    out mat3 TBN;
}vs_out;

struct Scene
{
    mat4 projection_view;
    mat4 model;
    vec3 view_pos;
};

struct ShaderConfig
{
    int enable_diffuse_map;
    int enable_specular_map;
    int enable_normal_map;
    int enable_gamma_correction;
};

uniform Scene u_scene;
uniform ShaderConfig u_config;

void main()
{
    vs_out.frag_pos = vec3((u_scene.model * vec4(vin_vertices, 1.0)).xyz);
    vs_out.uv = vin_texcoord;
    mat3 normal_matrix = transpose(inverse(mat3(u_scene.model)));
    vs_out.normals = normal_matrix * vin_normals;

    if(u_config.enable_normal_map == 1)
    {
        vec3 T = normalize(normal_matrix * vin_tangent);
        vec3 N = normalize(normal_matrix * vin_normals);
        T = normalize(T - dot(T, N) * N);
        vec3 B = cross(N, T);
    
        vs_out.TBN = transpose(mat3(T, B, N));    
    }
    else
    {
        vs_out.TBN = mat3(vec3(0.0,0.0,0.0), vec3(0.0,0.0,0.0), vec3(0.0,0.0,0.0));
    }

    gl_Position = u_scene.projection_view * vec4(vs_out.frag_pos, 1.0);
}