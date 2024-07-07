#vertex
#version 450 core
layout (location = 0) in vec3 vin_vertices;

uniform mat4 u_light_space_matrix;
uniform mat4 u_model;

void main()
{
    gl_Position = u_model * vec4(vin_vertices, 1.0);

} 

// ----------------------------------------------------------------------------- //
#geometry
#version 450 core
    
layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;
    
layout (std430, binding = 2) buffer LightSpaceMatrices
{
    mat4 ubo_light_space_matrices[];
};
    
void main()
{          
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = ubo_light_space_matrices[gl_InvocationID] * gl_in[i].gl_Position;
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