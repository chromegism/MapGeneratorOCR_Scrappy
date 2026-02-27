#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
layout(binding = 1) uniform MapDetails {
    ivec2 bufferSize;
    vec2 displaySize;
} md;

layout(location = 0) in float pos;
layout(location = 1) in float height;

layout(location = 0) out vec3 fragColor;

void main() {
    vec3 position = vec3(md.displaySize, 1.f) * 
        vec3(
            floor(gl_VertexIndex / md.bufferSize.x) / md.bufferSize.x - 0.5f, 
            mod(gl_VertexIndex, md.bufferSize.y) / md.bufferSize.y - 0.5f, 
            height
        );
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
    fragColor = vec3(height / 2.f + 0.5f);
}