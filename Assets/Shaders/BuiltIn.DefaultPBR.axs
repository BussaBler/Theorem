#type vertex
#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(push_constant) uniform PushConstants {
    uint viewportIndex;
    uint instanceOffset;
} pushConstants;

struct GlobalData {
    mat4 uView;
    mat4 uProjection;
    vec4 uCameraPosition;

    vec4 uAmbientColor;
    vec4 uDirectionalLightDir;
    vec4 uDirectionalLightColor;
};

layout(std140, set = 0, binding = 0) uniform GlobalDataBuffer {
    GlobalData viewports[10];
} globalDataBuffer;

struct MeshInstance {
    mat4 model;
};

layout(std430, set = 1, binding = 1) readonly buffer InstanceBuffer {
    MeshInstance instances[];
};

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec2 vTexCoord;

void main() {
    MeshInstance instance = instances[gl_InstanceIndex + pushConstants.instanceOffset];

    gl_Position = globalDataBuffer.viewports[pushConstants.viewportIndex].uProjection * globalDataBuffer.viewports[pushConstants.viewportIndex].uView * instance.model * vec4(aPosition, 1.0);
    vNormal = mat3(transpose(inverse(instance.model))) * aNormal;
    vTexCoord = aTexCoord;
}

#type fragment
#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec2 vTexCoord;

layout(push_constant) uniform PushConstants {
    uint viewportIndex;
    uint instanceOffset;
} pushConstants;

struct GlobalData {
    mat4 uView;
    mat4 uProjection;
    vec4 uCameraPosition;

    vec4 uAmbientColor;
    vec4 uDirectionalLightDir;
    vec4 uDirectionalLightColor;
};

layout(std140, set = 0, binding = 0) uniform GlobalDataBuffer {
    GlobalData viewports[10];
} globalDataBuffer;

layout(location = 0) out vec4 oColor;

void main() {
    vec3 albedo = vec3(1.0, 0.0, 0.0);

    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(-globalDataBuffer.viewports[pushConstants.viewportIndex].uDirectionalLightDir.xyz);

    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 diffuseColor = diffuse * globalDataBuffer.viewports[pushConstants.viewportIndex].uDirectionalLightColor.xyz;

    vec3 ambientColor = globalDataBuffer.viewports[pushConstants.viewportIndex].uAmbientColor.xyz;

    vec3 finalColor = ambientColor + diffuseColor;

    oColor = vec4(finalColor * albedo, 1.0);
}
