#type vertex
#version 460
#pragma shader_stage(vertex)

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec4 aData; // (x, y) = size
layout (location = 4) in vec4 aRadii;

layout(push_constant, std430) uniform PushConstants {
    mat4 proj;
};

layout (location = 0) out vec2 vUv;
layout (location = 1) out vec4 vColor;
layout (location = 2) out vec2 vSize;
layout (location = 3) out vec4 vRadii;

void main() {
    gl_Position = proj * vec4(aPos, 0.0, 1.0);
    vUv = (aUv - 0.5) * aData.xy;
    vColor = aColor;
    vSize = aData.xy * 0.5;
    vRadii = aRadii;
}

#type fragment
#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec2 vSize;
layout(location = 3) in vec4 vRadii;

layout(location = 0) out vec4 oColor;

float getRadius(vec2 p, vec4 radii) {
    if (p.x > 0.0) {
        return (p.y > 0.0) ? radii.y : radii.w;
    } else {
        return (p.y > 0.0) ? radii.x : radii.z;
    }
}

float sdRoundedBox(vec2 p, vec2 b, vec4 r) {
    float radius = getRadius(p, r);
    vec2 q = abs(p) - b + radius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

void main() {
    float dist = sdRoundedBox(vUv, vSize, vRadii);
    float alpha = 1.0 - smoothstep(-1.0, 1.0, dist);

    oColor = vec4(vColor.rgb * alpha, vColor.a * alpha);
}
