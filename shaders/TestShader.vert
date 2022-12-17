#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec4 v_color;
  
out vec4 f_color;

uniform mat4 u_view_projection;
uniform mat4 u_transform;

void main()
{
    gl_Position = u_view_projection * u_transform * vec4(v_pos, 1.0);
    f_color = v_color;
}