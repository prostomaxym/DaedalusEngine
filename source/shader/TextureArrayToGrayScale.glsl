#compute

#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2DArray u_input_array;
layout(binding = 1, rgba8) writeonly uniform image2D u_compute_image;

uniform int u_layer_index;

void main()
{
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(u_compute_image);

    if (gid.x >= size.x || gid.y >= size.y)
        return;

    // Normalized coordinates for sampling
    vec2 uv = vec2(gid) / vec2(size);

    float depth = texture(u_input_array, vec3(uv, u_layer_index)).r;

    vec4 color = vec4(vec3(depth), 1.0);
    imageStore(u_compute_image, gid, color);
}