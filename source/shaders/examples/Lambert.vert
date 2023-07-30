#version 430 core

layout (location = 0) in vec3 geo_Pos;
layout (location = 1) in vec2 geo_TexCoords;
layout (location = 2) in vec3 geo_Normal;

layout (std140) uniform EngineUBO
{
    mat4    ubo_Model;
    mat4    ubo_View;
    mat4    ubo_Projection;
    vec3    ubo_ViewPos;
    float   ubo_Time;
};

out VS_OUT
{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

void main()
{
    vs_out.FragPos      = vec3(ubo_Model * vec4(geo_Pos, 1.0));
    vs_out.Normal       = normalize(mat3(transpose(inverse(ubo_Model))) * geo_Normal);
    vs_out.TexCoords    = geo_TexCoords;

    gl_Position = ubo_Projection * ubo_View * vec4(vs_out.FragPos, 1.0);
}