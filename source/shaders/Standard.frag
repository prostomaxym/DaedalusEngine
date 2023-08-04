#version 450 core

in VS_OUT
{
    in vec3 frag_pos;
    in vec2 uv;
    in vec3 normals;
    in mat3 TBN;
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
    float constant;
	float linear;
	float quadratic;
    float cutoff_angle;
    int type; // 0 - directional, 1 - point, 2 - spot
};

layout (std140, binding = 0) uniform StaticLightUBO
{
    Light ubo_static_lights[];
};

layout (std140, binding = 1) uniform DynamicLightUBO
{
    Light ubo_dynamic_lights[];
};


struct Scene
{
    mat4 projection_view;
    mat4 model;
    vec3 view_pos;
};

struct ShaderConfig
{
    int enable_diffuse_map;
    int enable_specular_map;
    int enable_normal_map;
    int enable_gamma_correction;
};

uniform Material u_material; 
uniform Light u_light; 
uniform Scene u_scene;
uniform ShaderConfig u_config;

const float g_gamma_value = 2.2;

vec3 ApplyGamma(vec3 color, float gamma_value) 
{
    return pow(color, vec3(1.0 / gamma_value));
}

void main()
{
    const float tex_alpha = u_config.enable_diffuse_map == 1 ? texture(u_material.tex_diffuse, fs_in.uv).a : 1.0;

    // TODO: implement better way to show transparent objects in front of opaque objects
    if (tex_alpha < 0.99)
        discard;

    const vec3 diffuse_tex = u_config.enable_diffuse_map == 1 ? texture(u_material.tex_diffuse, fs_in.uv).rgb : vec3(1.0, 1.0, 1.0);

    const vec3 light_pos = u_config.enable_normal_map == 1 ? fs_in.TBN * u_light.position : u_light.position;
    const vec3 view_pos = u_config.enable_normal_map == 1 ? fs_in.TBN * u_scene.view_pos : u_scene.view_pos;
    const vec3 frag_pos = u_config.enable_normal_map == 1 ? fs_in.TBN * fs_in.frag_pos : fs_in.frag_pos;

    const vec3 normal = u_config.enable_normal_map == 1 ?
        normalize(vec3(texture(u_material.tex_normal, fs_in.uv).rgb) * 2.0 - 1.0) : fs_in.normals;

    // Ambient light
	const vec3 ambient = u_light.ambient * u_material.k_ambient * diffuse_tex;


    // Diffuse lighting
    const vec3 light_dir = normalize(light_pos);
    const vec3 diffuse_color = max(dot(normal, light_dir), 0.0) * u_material.k_diffuse;
    vec3 diffuse = u_light.diffuse * diffuse_color * diffuse_tex;


    // Specular lighting
    const vec3 spec_tex = u_config.enable_specular_map == 1 ? vec3(texture(u_material.tex_specular, fs_in.uv)) : vec3(1.0, 1.0, 1.0);
    const vec3 view_dir = normalize(view_pos - frag_pos);
    const vec3 halfway_dir = normalize(view_dir + light_dir);  
    const vec3 specular_color = pow(max(dot(normal, halfway_dir), 0.0), u_material.shininess) * u_material.k_specular;
    vec3 specular = u_light.specular * specular_color * spec_tex;    

    // Attenuation or Gain
    const float attenuation = ubo_static_lights[0].power;

    diffuse *= attenuation;
    specular *= attenuation;
    const vec3 light_color = ambient + diffuse + specular; 

    fout_color = u_config.enable_gamma_correction == 1 ? 
        vec4(ApplyGamma(light_color, g_gamma_value), tex_alpha) : 
        vec4(light_color, tex_alpha); 
}