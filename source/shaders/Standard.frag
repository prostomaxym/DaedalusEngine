#version 450 core

// -------------------------------------------Inputs/Outputs------------------------------------------------- //
in VS_OUT
{
    in vec3 frag_pos;
    in vec4 frag_pos_light_space;
    in vec2 uv;
    in vec3 normals;
    in mat3 TBN;
}fs_in;

out vec4 fout_color;

// ----------------------------------------------Structures-------------------------------------------------- //
struct GraphicConfig
{
    int enable_gamma_correction;
    float gamma_value;
};

struct ObjectData
{
    sampler2D tex_diffuse;
    sampler2D tex_specular;
    sampler2D tex_normal;
    mat4 model_mat;
    vec3 k_ambient;
    vec3 k_diffuse;
    vec3 k_specular;
    float shininess;
    int enable_diffuse_map;
    int enable_specular_map;
    int enable_normal_map;
};

struct Scene
{
    mat4 projection_view;
    vec3 view_pos;
};

struct Light 
{
    vec3 position;
    vec3 direction; 
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float power; 
    float constant;
	float linear;
	float quadratic;
    float cutoff_angle;
    float outer_cutoff_angle;
    int type; // 0 - directional, 1 - point, 2 - spot
};

// ------------------------------------------------Buffers----------------------------------------------------- //
layout (std140, binding = 0) uniform SceneUBO
{
    Scene ubo_scene;
};

layout (std140, binding = 1) uniform GraphicConfigUBO
{
    GraphicConfig ubo_graphic_config;
};

layout (std430, binding = 0) buffer StaticLightSSBO
{
    Light ssbo_static_lights[];
};

layout (std430, binding = 1) buffer DynamicLightSSBO
{
    Light ssbo_dynamic_lights[];
};

uniform ObjectData u_object;
uniform sampler2D u_shadow_map;

// ------------------------------------------------- Globals ------------------------------------------------ //
vec3 g_ambient_tex;
vec3 g_diffuse_tex;
vec3 g_spec_tex;
vec3 g_view_dir;
vec3 g_view_pos;
vec3 g_frag_pos;
vec3 g_normal;
float g_alpha_tex;
float g_shadow; 

// ------------------------------------------------- Functions ------------------------------------------------ //
vec3 BlinnPhong(vec3 p_light_ambient, vec3 p_light_diffuse, vec3 p_light_specular, vec3 p_light_dir, float p_luminosity)
{
    const vec3 halfway_dir = normalize(p_light_dir + g_view_dir);
    const float diffuse_coef  = max(dot(g_normal, p_light_dir), 0.0);
    const float specular_coef = pow(max(dot(g_normal, halfway_dir), 0.0), u_object.shininess);

    return p_luminosity * 
            (p_light_ambient * g_ambient_tex
            + (1.0 - g_shadow) * (p_light_diffuse * diffuse_coef * g_diffuse_tex
            + p_light_specular * specular_coef * g_spec_tex));
}

float CalculateAttenuation(vec3 p_light_position, float p_constant, float p_linear, float p_quadratic)
{
    const float distance = length(p_light_position - g_frag_pos);
    return 1.0 / (p_constant + p_linear * distance + p_quadratic * (distance * distance));
}

vec3 CalculateDirectionalLight(Light p_light)
{
    return BlinnPhong(p_light.ambient, p_light.diffuse, p_light.specular, p_light.direction, p_light.power);
}

vec3 CalculatePointLight(Light p_light)
{
    const vec3 light_direction  = normalize(p_light.position - g_frag_pos);
    const float luminosity      = CalculateAttenuation(p_light.position, p_light.constant, p_light.linear, p_light.quadratic);

    return BlinnPhong(p_light.ambient, p_light.diffuse, p_light.specular, light_direction, p_light.power * luminosity);
}

vec3 CalculateSpotLight(Light p_light)
{
    const vec3 light_direction  = normalize(p_light.position - g_frag_pos);
    const float luminosity      = CalculateAttenuation(p_light.position, p_light.constant, p_light.linear, p_light.quadratic);

    const float theta           = dot(light_direction, normalize(-p_light.direction)); 
    const float epsilon         = p_light.cutoff_angle - p_light.outer_cutoff_angle;
    const float spot_intensity = smoothstep(0.0, 1.0, (theta - p_light.outer_cutoff_angle) / epsilon);
    
     return BlinnPhong(p_light.ambient, p_light.diffuse, p_light.specular, light_direction, p_light.power * luminosity * spot_intensity);
}

float BilinearInterpolation(sampler2D shadowMap, vec2 texCoords) 
{
    ivec2 texelCoord = ivec2(texCoords * textureSize(shadowMap, 0));
    vec2 texelOffset = fract(texCoords * textureSize(shadowMap, 0));

    float topLeft = texture(shadowMap, (texelCoord + vec2(0.5, 0.5)) / textureSize(shadowMap, 0)).r;
    float topRight = texture(shadowMap, (texelCoord + vec2(1.5, 0.5)) / textureSize(shadowMap, 0)).r;
    float bottomLeft = texture(shadowMap, (texelCoord + vec2(0.5, 1.5)) / textureSize(shadowMap, 0)).r;
    float bottomRight = texture(shadowMap, (texelCoord + vec2(1.5, 1.5)) / textureSize(shadowMap, 0)).r;

    float topInterpolated = mix(topLeft, topRight, texelOffset.x);
    float bottomInterpolated = mix(bottomLeft, bottomRight, texelOffset.x);

    return mix(topInterpolated, bottomInterpolated, texelOffset.y);
}


float CalculateShadow(vec4 frag_pos_light_space, float softness) 
{
    vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
    proj_coords = proj_coords * 0.5 + 0.5;

    if (proj_coords.z > 1.0)
        return 0.0;

    const float closest_depth = texture(u_shadow_map, proj_coords.xy).r; 
    const float current_depth = proj_coords.z;

    // Enhanced bias calculation
    float bias = 0.005*tan(acos(dot(fs_in.normals, vec3(0.78, 1.0, 0.6))));
    bias = clamp(bias, 0.0, 0.01);

    float shadow = 0.0;
    const vec2 texel_size = 1.0 / textureSize(u_shadow_map, 0);
    
    // Percentage-closer filtering
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(x, y) * texel_size;
            float sampled_depth = BilinearInterpolation(u_shadow_map, proj_coords.xy + offset);
            float visibility = current_depth - bias > sampled_depth ? 0.7 : 0.0;
            shadow += visibility;
        }
    }

    shadow /= 25.0;

    return shadow;
}

vec3 ApplyGammaCorrection(vec3 color) 
{
    return pow(color, vec3(1.0 / ubo_graphic_config.gamma_value));
}

void main()
{
    g_alpha_tex = u_object.enable_diffuse_map == 1 ? texture(u_object.tex_diffuse, fs_in.uv).a : 1.0;

    // TODO: implement better way to show transparent objects in front of opaque objects
    if (g_alpha_tex < 0.99)
        discard;

    g_view_pos = ubo_scene.view_pos;
    g_frag_pos = fs_in.frag_pos;

    g_diffuse_tex = u_object.enable_diffuse_map == 1 ? texture(u_object.tex_diffuse, fs_in.uv).rgb : vec3(1.0, 1.0, 1.0);
    g_diffuse_tex *= u_object.k_diffuse;
    g_spec_tex = u_object.enable_specular_map == 1 ? vec3(texture(u_object.tex_specular, fs_in.uv)) : vec3(1.0, 1.0, 1.0);
    g_spec_tex *= u_object.k_specular;
    g_ambient_tex = u_object.k_ambient * g_diffuse_tex;

    g_view_dir = normalize(g_view_pos - g_frag_pos);
    g_shadow = CalculateShadow(fs_in.frag_pos_light_space, 1.0);    

    if(u_object.enable_normal_map == 1)
    {
        g_normal = texture(u_object.tex_normal, fs_in.uv).rgb;
        g_normal = normalize(g_normal * 2.0 - 1.0);   
        g_normal = normalize(fs_in.TBN * g_normal);
    }
    else
    {
        g_normal = fs_in.normals;
    }

    vec3 light_sum = vec3(0.0);

    for (int i = 0; i < ssbo_static_lights.length(); ++i)
    {
        switch(ssbo_static_lights[i].type)
        {
            case 0: light_sum += CalculateDirectionalLight(ssbo_static_lights[i]);   break;
            case 1: light_sum += CalculatePointLight(ssbo_static_lights[i]);         break;
            case 2: light_sum += CalculateSpotLight(ssbo_static_lights[i]);          break;
        }
    }

    for (int i = 0; i < ssbo_dynamic_lights.length(); ++i)
    {
        switch(ssbo_dynamic_lights[i].type)
        {
            case 0: light_sum += CalculateDirectionalLight(ssbo_dynamic_lights[i]);   break;
            case 1: light_sum += CalculatePointLight(ssbo_dynamic_lights[i]);          break;
            case 2: light_sum += CalculateSpotLight(ssbo_dynamic_lights[i]);          break;
        }
    }

    fout_color = ubo_graphic_config.enable_gamma_correction == 1 ? 
        vec4(ApplyGammaCorrection(light_sum), g_alpha_tex) : vec4(light_sum, g_alpha_tex);
}