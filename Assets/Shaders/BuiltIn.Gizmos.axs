#type vertex
#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(set = 0, binding = 0) uniform GlobalData {
    mat4 uView;
    mat4 uProjection;
    vec4 uCameraPosition;

    vec4 uAmbientColor;
    vec4 uDirectionalLightDir;
    vec4 uDirectionalLightColor;
} globalData;

layout(push_constant, std430) uniform PushConstants {
    mat4 uModel;
    vec4 uColor;
} pushConstants;

layout(location = 0) out vec4 vColor;

void main() {
    vColor = pushConstants.uColor;
    gl_Position = globalData.uProjection * globalData.uView * pushConstants.uModel * vec4(aPosition, 1.0);
}

#type fragment
#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vColor;
}
