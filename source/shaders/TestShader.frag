#version 330 core

uniform sampler2D tex;

in vec2 vout_uv;
out vec4 fout_color;

void main()
{
    fout_color = vec4(texture(tex, vout_uv).rgba);
} 