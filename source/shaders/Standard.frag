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
};

uniform Material u_material; 
uniform Light u_light; 
uniform Scene u_scene;
uniform ShaderConfig u_config;

void main()
{
    float tex_alpha;
    vec3 diffuse_tex;

    switch(u_config.diffuse_map_used)
    {
    case 0:
        {
            tex_alpha = 1.0;
            diffuse_tex = vec3(1.0, 1.0, 1.0);
            break;
        }
    case 1:
        {
            tex_alpha = texture(u_material.tex_diffuse, fs_in.uv).a;
            if (tex_alpha < 0.1)
                discard;

            diffuse_tex = texture(u_material.tex_diffuse, fs_in.uv).rgb;
            break;
        }
    }

    // Ambient light
	const vec3 ambient = u_light.ambient * u_material.k_ambient * diffuse_tex;


    // Diffuse lighting
    const vec3 light_dir = normalize(u_light.position - fs_in.frag_pos);
    const vec3 diffuse_color = max(dot(fs_in.normals, light_dir), 0.0) * u_material.k_diffuse;
    const vec3 diffuse = u_light.diffuse * diffuse_color * diffuse_tex;


    // Specular lighting
    vec3 spec_tex;

    switch(u_config.diffuse_map_used)
    {
    case 0:
        spec_tex = vec3(1.0, 1.0, 1.0);
        break;
    case 1:
        spec_tex = vec3(texture(u_material.tex_specular, fs_in.uv));
        break;
    }

    const vec3 view_dir = normalize(u_scene.view_pos - fs_in.frag_pos);
    const vec3 halfway_dir = normalize(view_dir + light_dir);  
    const vec3 specular_color = pow(max(dot(fs_in.normals, halfway_dir), 0.0), u_material.shininess) * u_material.k_specular;
    const vec3 specular = u_light.specular * specular_color * spec_tex;    

    fout_color = vec4(ambient + diffuse + specular, tex_alpha); 
} 