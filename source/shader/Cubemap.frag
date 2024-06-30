#version 450 core

in vec3 fin_tex_coords;

out vec4 fout_color;

uniform samplerCube u_cubemap;

void main()
{    
    fout_color = texture(u_cubemap, fin_tex_coords);
}