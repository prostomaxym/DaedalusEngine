#version 330 core

layout (location = 0) in vec3 v_vertices;
layout (location = 1) in vec2 v_texcoord;
layout (location = 2) in vec3 v_normals;
layout (location = 3) in vec3 v_tangent;
layout (location = 4) in vec3 v_bitangent;

uniform mat4 u_projection_view;
uniform mat4 u_transform;

out vec4 f_color;

void main()
{
    gl_Position = u_projection_view * u_transform * vec4(v_vertices, 1.0);
}