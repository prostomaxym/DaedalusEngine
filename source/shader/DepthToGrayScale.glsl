#compute

#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D u_input_image;
layout(binding = 1, rgba8) writeonly uniform image2D u_compute_image;

uniform float u_znear; 
uniform float u_zfar;
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * u_znear * u_zfar) / (u_zfar + u_znear - z * (u_zfar - u_znear));	
}

void main()
{
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(u_compute_image);

    if (gid.x >= size.x || gid.y >= size.y)
        return;

    vec2 uv = vec2(gid) / vec2(size);
    float depth = texture(u_input_image, uv).r;
    depth = LinearizeDepth(depth) / u_zfar;
    imageStore(u_compute_image, gid, vec4(vec3(depth), 1.0));
}