#version 450 core

in vec2 vout_uv;
in vec3 vout_specular;
in float vout_diff;
in float vout_distance;

uniform sampler2D tex;
uniform float u_ambient_strength;
uniform float u_light_power;

out vec4 fout_color;

void main()
{
	const vec3 tex_color = vec3(texture(tex, vout_uv).rgb);
	const vec3 ambient = u_ambient_strength * tex_color;
    const vec3 diffuse = vout_diff * tex_color;
    //const float tex_alpha = texture(tex, vout_uv).a;
    
    fout_color = vec4(ambient + diffuse * u_light_power / vout_distance + vout_specular, 1.0);
} 