#version 330 core

uniform sampler2D tex;

in vec2 fin_uv;
out vec4 fout_color;

void main()
{
    fout_color = vec4(texture(tex, fin_uv).rgb, 1.0);
} 