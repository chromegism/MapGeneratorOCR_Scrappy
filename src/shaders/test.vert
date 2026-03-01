#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 0, binding = 1) uniform MapDetails {
    ivec2 bufferSize;
    vec2 displaySize;
} md;

layout(set = 0, binding = 2) uniform sampler2D heightMap;

layout(location = 0) in vec2 pos;
layout(location = 0) out vec3 fragColor;

void main() {
    float height = textureLod(heightMap, pos, 0.0).r;

    vec3 pos = 
        vec3(md.displaySize, 1.f) * 
        vec3(
            pos.x - 0.5,
            pos.y - 0.5,
            height
        );

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
    fragColor = vec3(height * 0.5 + 0.5);
}