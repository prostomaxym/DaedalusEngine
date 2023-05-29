#version 330 core

layout (location = 0) in vec3 vin_vertices;
layout (location = 1) in vec2 vin_texcoord;
layout (location = 2) in vec3 vin_normals;
layout (location = 3) in vec3 vin_tangent;
layout (location = 4) in vec3 vin_bitangent;

uniform mat4 u_projection_view;
uniform mat4 u_transform;

out vec2 vout_uv;

void main()
{
    gl_Position = u_projection_view * u_transform * vec4(vin_vertices, 1.0);
}