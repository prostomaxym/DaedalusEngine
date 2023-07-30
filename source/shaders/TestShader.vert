#version 450 core

layout (location = 0) in vec3 vin_vertices;
layout (location = 1) in vec2 vin_texcoord;
layout (location = 2) in vec3 vin_normals;
layout (location = 3) in vec3 vin_tangent;
layout (location = 4) in vec3 vin_bitangent;

uniform mat4 u_projection_view;
uniform mat4 u_transform;
uniform vec3 u_light_pos;
uniform vec3 u_view_pos;
uniform vec3 u_light_color;
uniform float u_spec_strength;
uniform float u_shininess;
uniform float u_light_power;

out vec2 vout_uv;
out vec3 vout_specular;
out float vout_diff;
out float vout_distance;

void main()
{
    vec4 world_pos = u_transform * vec4(vin_vertices, 1.0);

    vec3 normals = mat3(transpose(inverse(u_transform))) * vin_normals; //Expensive - can be optimized

    vec3 light_dir = u_light_pos - world_pos.xyz;
	vout_distance = length(light_dir);
	light_dir = normalize(light_dir);
    vec3 normal = normalize(normals);
   
    vec3 view_dir = normalize(u_view_pos - world_pos.xyz);
    vec3 halfway_dir = normalize(light_dir + view_dir);  
    float spec = pow(max(dot(normal, halfway_dir), 0.0), u_shininess);

    vout_uv = vin_texcoord;
    vout_diff = max(dot(light_dir, normal), 0.0);
    vout_specular = u_spec_strength * spec * u_light_power / vout_distance * u_light_color;
    gl_Position = u_projection_view * world_pos;
}