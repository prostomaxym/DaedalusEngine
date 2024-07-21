#vertex
#version 450 core

layout (location = 0) in vec3 vin_vertices;
layout (location = 1) in vec2 vin_texcoord;

out vec2 vout_uv;

void main()
{
    vout_uv = vin_texcoord;
    gl_Position = vec4(vin_vertices, 1.0);
}


// ----------------------------------------------------------------------------- //
#fragment
#version 450 core
#include "Scene.hglsl"
#include "LightSSBO.hglsl"

// -------------------------------------------Inputs/Outputs------------------------------------------------- //

in vec2 vout_uv;
out vec4 fout_color;

uniform sampler2D u_gPosition;
uniform sampler2D u_gNormal;
uniform sampler2D u_gAmbient;
uniform sampler2D u_gSpec;
uniform sampler2D u_gAlbedo;
uniform sampler2D u_gShininess;
uniform sampler2D u_gSSAO;
uniform sampler2DArray u_shadowmaps;

// ------------------------------------------------- Globals ------------------------------------------------ //
vec3 g_ambient_tex = vec3(0.0, 0.0, 0.0);
vec3 g_diffuse_tex = vec3(0.0, 0.0, 0.0);
vec3 g_spec_tex = vec3(0.0, 0.0, 0.0);
vec3 g_view_dir = vec3(0.0, 0.0, 0.0);
vec3 g_view_pos = vec3(0.0, 0.0, 0.0);
vec3 g_frag_pos = vec3(0.0, 0.0, 0.0);
vec3 g_normal = vec3(0.0, 0.0, 0.0);
float g_shininess = 1.0;


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

// Percentage-closer filtering
float CalculateShadowPCF(sampler2DArray shadow_map, vec2 proj_coords, int shadow_map_index, float test_depth)
{
    float shadow = 0.0;
    const vec2 texel_size = 1.0 / vec2(textureSize(shadow_map, 0));

    const int off = (ubo_graphic_config.PCF_multiplier - 1) / 2;
    for (int x = -off; x <= off; ++x)
    {
        for (int y = -off; y <= off; ++y)
        {
            vec2 offset = vec2(x, y) * texel_size;
            float sampled_depth = BilinearInterpolation(shadow_map, proj_coords + offset, shadow_map_index);
            //float sampled_depth = texture(shadow_map, vec3(proj_coords.xy + vec2(x, y) * texel_size, shadow_map_index)).r;

            float visibility = test_depth > sampled_depth ? 0.8 : 0.0;
            shadow += visibility;
        }
    }

    shadow /= pow(ubo_graphic_config.PCF_multiplier, 2);
    return shadow;
}

float CalculateCascadePlaneDistance(float znear, float zfar, int cascade_plane_inx, int number_of_cascades)
{
    const float percent = float(cascade_plane_inx) / float(number_of_cascades);
    const float cascade_exponent = ubo_graphic_config.csm_exponent;
    return znear + (zfar - znear) * pow(percent, cascade_exponent);
}

float CalculateShadowBias(vec3 light_dir, int light_type, int number_of_cascades, int cascade_ind)
{
    float bias = 0.0;

    // TODO: Rework bias, not working completely for all light sources
    if (light_type == POINT_LIGHT_SOURCE)
    {
        bias = 0.0001;
    }
    else
    {
        //bias = 0.005 * tan(acos(dot(g_normal, light_dir)));
        //bias = clamp(bias, 0.0, 0.01);
       
        //if (number_of_cascades > 1)
        //{
        //    const float bias_modifier = 0.5f;
        //    bias *= 1 / (CalculateCascadePlaneDistance(ubo_scene.near_plane, ubo_scene.far_plane, cascade_ind, number_of_cascades) * bias_modifier);
        //}

         bias = 0.0001;
    }

    return bias;
}

int FindCascadeIndex(Light p_light)
{
    if (p_light.number_of_shadow_cascades <= 1)
        return 0;

    vec4 frag_pos_view_space = ubo_scene.view * vec4(g_frag_pos, 1.0);
    float view_depth = abs(frag_pos_view_space.z);

    int cascade = p_light.number_of_shadow_cascades - 1;
    for (int i = 0; i < p_light.number_of_shadow_cascades; ++i)
    {
        if (view_depth < CalculateCascadePlaneDistance(ubo_scene.near_plane, ubo_scene.far_plane, i + 1, p_light.number_of_shadow_cascades))
        {
            cascade = i;
            break;
        }
    }

    return cascade;
}

int FindShadowCubeIndex(vec3 light_pos)
{
    int face_index = -1;
    vec3 frag_to_light = g_frag_pos - light_pos;

    if (abs(frag_to_light.x) > abs(frag_to_light.y) && abs(frag_to_light.x) > abs(frag_to_light.z))
        face_index = frag_to_light.x > 0.0 ? 0 : 1;
    else if (abs(frag_to_light.y) > abs(frag_to_light.x) && abs(frag_to_light.y) > abs(frag_to_light.z))
        face_index = frag_to_light.y > 0.0 ? 2 : 3;
    else
        face_index = frag_to_light.z > 0.0 ? 4 : 5;

    return face_index;
}

float CalculateShadow(Light p_light, sampler2DArray shadow_map)
{
    const int cascade_ind = p_light.type == POINT_LIGHT_SOURCE ? FindShadowCubeIndex(p_light.position) : FindCascadeIndex(p_light);
    const vec4 frag_pos_light_space = ubo_light_proj_view[p_light.shadowmap_index + cascade_ind] * vec4(g_frag_pos, 1.0);
    const vec3 proj_coords = 0.5 * (frag_pos_light_space.xyz / frag_pos_light_space.w) + 0.5;

    if (proj_coords.z > 1.0)
        return 1.0;

    const float bias = CalculateShadowBias(p_light.direction, p_light.type, p_light.number_of_shadow_cascades, cascade_ind);
    const float current_depth = proj_coords.z;
    const float test_depth = current_depth - bias;

    float shadow = CalculateShadowPCF(shadow_map, proj_coords.xy, p_light.shadowmap_index + cascade_ind, test_depth);

    return 1.0 - shadow;
}


// ------------------------------------------------- Core Lighting Functions ------------------------------------------------ //
vec3 BlinnPhong(Light p_light, vec3 p_light_dir, float p_luminosity)
{
    const vec3 halfway_dir = normalize(p_light_dir + g_view_dir);
    const float diffuse_coef  = max(dot(g_normal, p_light_dir), 0.0);
    const float specular_coef = pow(max(dot(g_normal, halfway_dir), 0.0), g_shininess);

    //float visability = p_light.cast_shadows ? CalculateShadow(p_light, u_shadowmaps) : 1.0;
    //float visability_trip = pow(visability, 3.0);

    float visability = 1.0;
    float visability_trip = 1.0;
    return p_luminosity *
            (p_light.ambient * g_ambient_tex
            + visability * p_light.diffuse * diffuse_coef * g_diffuse_tex
            + visability_trip * p_light.specular * specular_coef * g_spec_tex);
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
    g_frag_pos = texture(u_gPosition, vout_uv).rgb;
    g_normal = texture(u_gNormal, vout_uv).rgb;
    g_spec_tex = texture(u_gSpec, vout_uv).rgb;
    g_diffuse_tex = texture(u_gAlbedo, vout_uv).rgb;
    g_ambient_tex = texture(u_gAmbient, vout_uv).rgb;

    if (ubo_graphic_config.enable_ssao)
    {
        float ambient_occlusion = texture(u_gSSAO, vout_uv).r;
        g_ambient_tex *= ambient_occlusion;
    }

    g_shininess = 1.0 / texture(u_gShininess, vout_uv).r;
    g_view_pos = ubo_scene.view_pos;
    g_view_dir = normalize(g_view_pos - g_frag_pos);

    vec3 light_sum = vec3(0.0);
    int i = 0;
    for (i = 0; i < 4; ++i)
    {
        switch(ssbo_lights[i].type)
        {
            case DIRECTIONAL_LIGHT_SOURCE: light_sum += CalculateDirectionalLight(ssbo_lights[i]);  break;
            case POINT_LIGHT_SOURCE: light_sum += CalculatePointLight(ssbo_lights[i]);              break;
            case SPOT_LIGHT_SOURCE: light_sum += CalculateSpotLight(ssbo_lights[i]);                break;
        }
    }

    fout_color = ubo_graphic_config.enable_gamma_correction ?
      vec4(ApplyGammaCorrection(light_sum), 1.0) : vec4(light_sum, 1.0);

    //fout_color = vec4(g_diffuse_tex, 1.0);
    //float test = i;
   // fout_color = vec4(test / 3.0, test / 3.0, test / 3.0, 1.0);
}
