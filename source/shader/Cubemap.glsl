#vertex
#version 450 core
layout (location = 0) in vec3 vin_vertices;

out vec3 fin_tex_coords;

uniform mat4 u_projection_view;

void main()
{
    fin_tex_coords = vin_vertices;
    vec4 pos = u_projection_view * vec4(vin_vertices, 1.0);
    gl_Position = pos.xyww;
}


// ----------------------------------------------------------------------------- //
#fragment
#version 450 core

in vec3 fin_tex_coords;

out vec4 fout_color;

uniform samplerCube u_cubemap;

void main()
{    
    fout_color = texture(u_cubemap, fin_tex_coords);
}