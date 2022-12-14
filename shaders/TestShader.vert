#version 330 core
layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec4 v_colour;
  
out vec4 f_colour;

void main()
{
    gl_Position = vec4(v_pos, 1.0);
}