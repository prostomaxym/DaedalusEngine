#vertex
#version 450 core
layout (location = 0) in vec3 vin_vertices;

uniform mat4 u_model;

void main()
{
    gl_Position = u_model * vec4(vin_vertices, 1.0);
} 

// ----------------------------------------------------------------------------- //
#geometry
#version 450 core
#include "LightSSBO.hglsl"

// If number of light sources * cascades exceeds this number - it will not generate shadowmap
// Right now I will remain this hard coded
layout(triangles, invocations = 32) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{          
    if (gl_InvocationID >= ubo_light_proj_view.length())
        return;
        
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = ubo_light_proj_view[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}  


// ----------------------------------------------------------------------------- //
#fragment
#version 450 core

void main()
{             
    // gl_FragDepth = gl_FragCoord.z;
}  