#type vertex
#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec3 aPosition;

layout(location = 0) out vec3 vPosition;

layout(set = 0, binding = 0) uniform GlobalData {
    mat4 uView;
    mat4 uProjection;
    vec4 uCameraPosition;

    vec4 uAmbientColor;
    vec4 uDirectionalLightDir;
    vec4 uDirectionalLightColor;
} globalData;

void main() {
    vPosition = aPosition;
    mat3 rotView = mat3(globalData.uView);
    mat4 rotViewProj = globalData.uProjection * mat4(rotView);
    gl_Position = (rotViewProj * vec4(aPosition, 1.0)).xyww;
}

#type fragment
#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec3 vPosition;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 dir = normalize(vPosition);

    const vec3 zenithColor = vec3(0.1, 0.2, 0.5); // Deep Blue
    const vec3 horizonColor = vec3(0.6, 0.7, 0.8); // Light Blue/Grey
    const vec3 groundColor = vec3(0.2, 0.2, 0.2); // Dark Grey

    float factor = dir.y;

    vec3 finalColor;
    if (factor > 0.0) {
        finalColor = mix(horizonColor, zenithColor, factor);
    } else {
        finalColor = mix(horizonColor, groundColor, abs(factor));
    }

    outColor = vec4(finalColor, 1.0);
}
