#version 450 core

in VS_OUT
{
    in vec3 frag_pos;
    in vec2 uv;
    in vec3 normals;
}fs_in;

out vec4 fout_color;

struct Material
{
    sampler2D tex_diffuse;
    sampler2D tex_specular;
    sampler2D tex_normal;
    vec3 k_ambient;
    vec3 k_diffuse;
    vec3 k_specular;
    float shininess;
};

struct Light 
{
    vec3 position; 
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float power; 
};

struct Scene
{
    mat4 projection_view;
    mat4 model;
    vec3 view_pos;
};

struct ShaderConfig
{
    int diffuse_map_used;
    int specular_map_used;
    int normal_map_used;
    int gamma_correction_used;
};

uniform Material u_material; 
uniform Light u_light; 
uniform Scene u_scene;
uniform ShaderConfig u_config;

void main()
{
    const float tex_alpha = u_config.diffuse_map_used == 1 ? texture(u_material.tex_diffuse, fs_in.uv).a : 1.0;
    const vec3 diffuse_tex = u_config.diffuse_map_used == 1 ? texture(u_material.tex_diffuse, fs_in.uv).rgb : vec3(1.0, 1.0, 1.0);

    // Ambient light
	const vec3 ambient = u_light.ambient * u_material.k_ambient * diffuse_tex;


    // Diffuse lighting
    const vec3 light_dir = normalize(u_light.position);
    const vec3 diffuse_color = max(dot(fs_in.normals, light_dir), 0.0) * u_material.k_diffuse;
    vec3 diffuse = u_light.diffuse * diffuse_color * diffuse_tex;


    // Specular lighting
    const vec3 spec_tex = u_config.diffuse_map_used == 1 ? vec3(texture(u_material.tex_specular, fs_in.uv)) : vec3(1.0, 1.0, 1.0);
    const vec3 view_dir = normalize(u_scene.view_pos - fs_in.frag_pos);
    const vec3 halfway_dir = normalize(view_dir + light_dir);  
    const vec3 specular_color = pow(max(dot(fs_in.normals, halfway_dir), 0.0), u_material.shininess) * u_material.k_specular;
    vec3 specular = u_light.specular * specular_color * spec_tex;    

    // Attenuation
    const float attenuation = u_light.power;

    diffuse *= attenuation;
    specular *= attenuation;

    const vec3 light_color = ambient + diffuse + specular; 

    fout_color = u_config.gamma_correction_used == 1 ? vec4(pow(light_color, vec3(2.0/2.2)), tex_alpha) : vec4(light_color, tex_alpha); 
}