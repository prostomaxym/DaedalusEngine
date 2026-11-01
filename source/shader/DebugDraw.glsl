#vertex
#version 450 core
#include "Scene.hglsl"

layout (location = 0) in vec3 vin_vertices;
layout (location = 1) in vec3 vin_color;
out vec3 color;

void main()
{
	color = vin_color;
	gl_Position = ubo_scene.projection_view * vec4(vin_vertices, 1.0);
}

#fragment
#version 450 core

in vec3 color;
layout (location = 0) out vec4 fout_color;

void main()
{
	fout_color = vec4(color, 1.0);
}
