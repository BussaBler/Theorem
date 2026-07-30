#type vertex
#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 2) in vec4 aColor;

layout(push_constant, std430) uniform PushConstants {
    mat4 proj;
};

layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;

void main() {
    gl_Position = proj * vec4(aPos, 0.0, 1.0);
    vUv = aUv;
    vColor = aColor;
}

#type fragment
#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform texture2D uImageTexture;
layout(set = 0, binding = 1) uniform sampler uImageSampler;

void main() {
    oColor = vColor * texture(sampler2D(uImageTexture, uImageSampler), vUv);
}
