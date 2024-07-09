#vertex
#version 450 core

layout (location = 0) in vec3 vin_vertices;
layout (location = 1) in vec2 vin_texcoord;
layout (location = 2) in vec3 vin_normals;
layout (location = 3) in vec3 vin_tangent;
layout (location = 4) in vec3 vin_bitangent;

out VS_OUT
{
    out vec3 frag_pos;
    out vec2 uv;
    out vec3 normals;
    out mat3 TBN;
}vs_out;

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
    bool enable_diffuse_map;
    bool enable_specular_map;
    bool enable_normal_map;
};

struct Scene
{
    mat4 projection_view;
    mat4 view;
    vec3 view_pos;
    vec3 cascade_distances;
};

layout (std140, binding = 0) uniform SceneUBO
{
    Scene ubo_scene;
};

uniform ObjectData u_object; 

void main()
{
    vs_out.frag_pos = vec3((u_object.model_mat * vec4(vin_vertices, 1.0)).xyz);
    vs_out.uv = vin_texcoord;
    mat3 normal_matrix = transpose(inverse(mat3(u_object.model_mat)));
    vs_out.normals = normalize(normal_matrix * vin_normals);

    if (u_object.enable_normal_map)
    {
        vs_out.TBN = mat3
        (
            normalize(vec3(u_object.model_mat * vec4(vin_tangent, 0.0))),
            normalize(vec3(u_object.model_mat * vec4(vin_bitangent, 0.0))),
            normalize(vec3(u_object.model_mat * vec4(vin_normals, 0.0)))
        );   
    }
    else
    {
        vs_out.TBN = mat3(vec3(0.0,0.0,0.0), vec3(0.0,0.0,0.0), vec3(0.0,0.0,0.0));
    }

    gl_Position = ubo_scene.projection_view * vec4(vs_out.frag_pos, 1.0);
}


// ----------------------------------------------------------------------------- //
#fragment
#version 450 core


// -------------------------------------------Inputs/Outputs------------------------------------------------- //
in VS_OUT
{
    in vec3 frag_pos;
    in vec2 uv;
    in vec3 normals;
    in mat3 TBN;
}fs_in;

out vec4 fout_color;


// ----------------------------------------------Structures-------------------------------------------------- //
struct GraphicConfig
{
    bool enable_gamma_correction;
    float gamma_value;
    int PCF_multiplier;
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
    bool enable_diffuse_map;
    bool enable_specular_map;
    bool enable_normal_map;
};

struct Scene
{
    mat4 projection_view;
    mat4 view;
    vec3 view_pos;
    vec3 cascade_distances;
};

struct Light 
{
    vec3 position;
    int type; // 0 - directional, 1 - point, 2 - spot
    vec3 direction;
    bool cast_shadows;
    vec3 ambient;
    int shadowmap_index;
    vec3 diffuse;
    int number_of_shadow_cascades;
    vec3 specular;
    float power; 
    float constant;
	float linear;
	float quadratic;
    float cutoff_angle;
    float outer_cutoff_angle;
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

layout (std430, binding = 2) buffer LightSpaceMatrices
{
    mat4 ubo_light_space_matrices[];
};

uniform ObjectData u_object;
uniform sampler2DArray u_shadowmaps;

// ------------------------------------------------- Globals ------------------------------------------------ //
vec3 g_ambient_tex = vec3(0.0, 0.0, 0.0);
vec3 g_diffuse_tex = vec3(0.0, 0.0, 0.0);
vec3 g_spec_tex = vec3(0.0, 0.0, 0.0);
vec3 g_view_dir = vec3(0.0, 0.0, 0.0);
vec3 g_view_pos = vec3(0.0, 0.0, 0.0);
vec3 g_frag_pos = vec3(0.0, 0.0, 0.0);
vec3 g_normal = vec3(0.0, 0.0, 0.0);
float g_alpha_tex = 0;


// ------------------------------------------------- General Functions ------------------------------------------------ //
vec3 ApplyGammaCorrection(vec3 color) 
{
    return pow(color, vec3(1.0 / ubo_graphic_config.gamma_value));
}

float BilinearInterpolation(sampler2DArray shadow_map, vec2 tex_coords, int layer) 
{
    ivec2 texel_coord = ivec2(tex_coords * vec2(textureSize(shadow_map, 0)));
    vec2 texel_offset = fract(tex_coords * vec2(textureSize(shadow_map, 0)));

    float top_left = texture(shadow_map, vec3((texel_coord + vec2(0.5, 0.5)) / vec2(textureSize(shadow_map, 0)), layer)).r;
    float top_right = texture(shadow_map, vec3((texel_coord + vec2(1.5, 0.5)) / vec2(textureSize(shadow_map, 0)), layer)).r;
    float bottom_left = texture(shadow_map, vec3((texel_coord + vec2(0.5, 1.5)) / vec2(textureSize(shadow_map, 0)), layer)).r;
    float bottom_right = texture(shadow_map, vec3((texel_coord + vec2(1.5, 1.5)) / vec2(textureSize(shadow_map, 0)), layer)).r;

    float top_interpolated = mix(top_left, top_right, texel_offset.x);
    float bottom_interpolated = mix(bottom_left, bottom_right, texel_offset.x);

    return mix(top_interpolated, bottom_interpolated, texel_offset.y);
}

int FindCascadeIndex(float depth_value)
{
    int layer = 2;
    for (int i = 0; i < 3; ++i)
    {
        if (depth_value < ubo_scene.cascade_distances[i])
        {
            layer = i;
            break;
        }
    }
    return layer;
}

float CalculateShadow(Light p_light, sampler2DArray shadow_map) 
{
    vec4 frag_pos_view_space = ubo_scene.view * vec4(fs_in.frag_pos, 1.0);
    float view_depth = abs(frag_pos_view_space.z);
    int cascade_ind = p_light.number_of_shadow_cascades > 1 ? FindCascadeIndex(view_depth) : 0;

    vec4 frag_pos_light_space = ubo_light_space_matrices[p_light.shadowmap_index + cascade_ind] * vec4(fs_in.frag_pos, 1.0);
    vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
    proj_coords = proj_coords * 0.5 + 0.5;

    if (proj_coords.z > 1.0)
        return 1.0;

    float bias = 0.005 * tan(acos(dot(fs_in.normals, p_light.direction)));
    bias = clamp(bias, 0.0, 0.01);
    bias = 0.0;
    if (p_light.number_of_shadow_cascades > 1)
    {
        const float bias_modifier = 0.5f;
        bias *= 1 / (ubo_scene.cascade_distances[cascade_ind] * bias_modifier);
    }

    float current_depth = proj_coords.z;
    const float test_depth = current_depth - bias;
       
    float shadow = 0.0;
    const vec2 texel_size = 1.0 / vec2(textureSize(shadow_map, 0));
    
    // Percentage-closer filtering
    const int off = (ubo_graphic_config.PCF_multiplier - 1) / 2;
    for (int x = -off; x <= off; ++x)
    {
        for (int y = -off; y <= off; ++y)
        {
            vec2 offset = vec2(x, y) * texel_size;
            float sampled_depth = BilinearInterpolation(shadow_map, proj_coords.xy + offset, p_light.shadowmap_index + cascade_ind);
            //float sampled_depth = texture(shadow_map, vec3(proj_coords.xy + vec2(x, y) * texel_size, p_light.shadowmap_index + cascade_ind)).r;

            float visibility = test_depth > sampled_depth ? 0.8 : 0.0;
            shadow += visibility;
        }
    }

    shadow /= pow(ubo_graphic_config.PCF_multiplier, 2);

    return 1.0 - shadow;
}


// ------------------------------------------------- Core Lighting Functions ------------------------------------------------ //
vec3 BlinnPhong(Light p_light, vec3 p_light_dir, float p_luminosity)
{
    const vec3 halfway_dir = normalize(p_light_dir + g_view_dir);
    const float diffuse_coef  = max(dot(g_normal, p_light_dir), 0.0);
    const float specular_coef = pow(max(dot(g_normal, halfway_dir), 0.0), u_object.shininess);

    float visability = p_light.cast_shadows ? CalculateShadow(p_light, u_shadowmaps) : 1.0;

    return p_luminosity *
            (p_light.ambient * g_ambient_tex
            + visability * (p_light.diffuse * diffuse_coef * g_diffuse_tex
            + p_light.specular * specular_coef * g_spec_tex));
}

float CalculateAttenuation(vec3 p_light_position, float p_constant, float p_linear, float p_quadratic)
{
    const float distance = length(p_light_position - g_frag_pos);
    return 1.0 / (p_constant + p_linear * distance + p_quadratic * (distance * distance));
}

vec3 CalculateDirectionalLight(Light p_light)
{
    return BlinnPhong(p_light, p_light.direction, p_light.power);
}

vec3 CalculatePointLight(Light p_light)
{
    const vec3 light_direction  = normalize(p_light.position - g_frag_pos);
    const float luminosity      = CalculateAttenuation(p_light.position, p_light.constant, p_light.linear, p_light.quadratic);

    return BlinnPhong(p_light, light_direction, p_light.power * luminosity);
}

vec3 CalculateSpotLight(Light p_light)
{
    const vec3 light_direction  = normalize(p_light.position - g_frag_pos);
    const float luminosity      = CalculateAttenuation(p_light.position, p_light.constant, p_light.linear, p_light.quadratic);

    const float theta           = dot(light_direction, normalize(-p_light.direction)); 
    const float epsilon         = p_light.cutoff_angle - p_light.outer_cutoff_angle;
    const float spot_intensity  = smoothstep(0.0, 1.0, (theta - p_light.outer_cutoff_angle) / epsilon);
    
     return BlinnPhong(p_light, light_direction, p_light.power * luminosity * spot_intensity);
}

void main()
{
    g_alpha_tex = u_object.enable_diffuse_map ? texture(u_object.tex_diffuse, fs_in.uv).a : 1.0;

    // TODO: implement better way to show transparent objects in front of opaque objects
    if (g_alpha_tex < 0.99)
        discard;

    g_view_pos = ubo_scene.view_pos;
    g_frag_pos = fs_in.frag_pos;

    g_diffuse_tex = u_object.enable_diffuse_map ? texture(u_object.tex_diffuse, fs_in.uv).rgb : vec3(1.0, 1.0, 1.0);
    g_diffuse_tex *= u_object.k_diffuse;
    g_spec_tex = u_object.enable_specular_map ? vec3(texture(u_object.tex_specular, fs_in.uv)) : vec3(1.0, 1.0, 1.0);
    g_spec_tex *= u_object.k_specular;
    g_ambient_tex = u_object.k_ambient * g_diffuse_tex;

    g_view_dir = normalize(g_view_pos - g_frag_pos);  

    if (u_object.enable_normal_map)
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
            case 1: light_sum += CalculatePointLight(ssbo_dynamic_lights[i]);         break;
            case 2: light_sum += CalculateSpotLight(ssbo_dynamic_lights[i]);          break;
        }
    }

    fout_color = ubo_graphic_config.enable_gamma_correction ?
        vec4(ApplyGammaCorrection(light_sum), g_alpha_tex) : vec4(light_sum, g_alpha_tex);
}
