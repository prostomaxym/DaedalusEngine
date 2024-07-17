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

uniform sampler2D u_tex_input;

void main()
{
    vec2 texel_size = 1.0 / vec2(textureSize(u_tex_input, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x)
    {
        for (int y = -2; y < 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(u_tex_input, vout_uv + offset).r;
        }
    }

    fout_color = result / (4.0 * 4.0);
}