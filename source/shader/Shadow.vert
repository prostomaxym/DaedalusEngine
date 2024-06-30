#version 450 core
layout (location = 0) in vec3 vin_vertices;

uniform mat4 u_light_space_matrix;
uniform mat4 u_model;

void main()
{
    gl_Position = u_light_space_matrix * u_model * vec4(vin_vertices, 1.0);
}  