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

in vec2 vout_uv;
out float fout_color;

uniform sampler2D u_gPosition;
uniform sampler2D u_gNormal;
uniform sampler2D u_tex_noise;

uniform vec3 u_samples[64];
uniform mat4 u_projection;
uniform mat4 u_view;

const int kernel_size = 64;
const float radius = 0.5;
const float bias = 0.025;

const vec2 noise_scale = vec2(2560.0/4.0, 1440.0/4.0);

void main()
{
    vec4 frag_pos = u_view * texture(u_gPosition, vout_uv);
    vec3 normal = mat3(u_view) * texture(u_gNormal, vout_uv).rgb;
    vec3 random_vec = normalize(texture(u_tex_noise, vout_uv * noise_scale).xyz);

    vec3 tangent = normalize(random_vec - normal * dot(random_vec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernel_size; ++i)
    {
        vec3 sample_pos = TBN * u_samples[i];
        sample_pos = frag_pos.xyz + sample_pos * radius;

        vec4 offset = vec4(sample_pos, 1.0);
        offset = u_projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sample_depth = vec3(u_view * texture(u_gPosition, offset.xy)).z;

        float range_check = smoothstep(0.0, 1.0, radius / abs(frag_pos.z - sample_depth));
        occlusion += (sample_depth >= sample_pos.z + bias ? 1.0 : 0.0) * range_check;
    }

    occlusion = 1.0 - (occlusion / kernel_size);
    fout_color = occlusion;
}