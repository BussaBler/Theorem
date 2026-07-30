#type vertex
#version 460
#pragma shader_stage(vertex)

const float GRID_SIZE = 5000.0;
const vec3 vertices[6] = vec3[](
        vec3(-1.0, 0.0, -1.0), vec3(1.0, 0.0, -1.0), vec3(1.0, 0.0, 1.0),
        vec3(-1.0, 0.0, 1.0), vec3(-1.0, 0.0, -1.0), vec3(1.0, 0.0, 1.0)
    );

layout(set = 0, binding = 0) uniform GlobalData {
    mat4 uView;
    mat4 uProjection;
    vec4 uCameraPosition;

    vec4 uAmbientColor;
    vec4 uDirectionalLightDir;
    vec4 uDirectionalLightColor;
} globalData;

layout(location = 0) out vec3 vWorldPosition;

void main() {
    vec3 localPos = vertices[gl_VertexIndex] * GRID_SIZE;

    vec3 worldPos = localPos;
    worldPos.x += globalData.uCameraPosition.x;
    worldPos.z += globalData.uCameraPosition.z;
    vWorldPosition = worldPos;

    gl_Position = globalData.uProjection * globalData.uView * vec4(worldPos, 1.0);
}

#type fragment
#version 460
#pragma shader_stage(fragment)

layout(set = 0, binding = 0) uniform GlobalData {
    mat4 uProjection;
    mat4 uView;
    vec4 uCameraPosition;

    vec4 uAmbientColor;
    vec4 uDirectionalLightDir;
    vec4 uDirectionalLightColor;
} globalData;

layout(location = 0) in vec3 vWorldPosition;

layout(location = 0) out vec4 outColor;

float drawGrid(vec2 coord, float spacing) {
    vec2 gridCoord = coord / spacing;

    vec2 dWidth = fwidth(gridCoord);
    vec2 grid = abs(fract(gridCoord - 0.5) - 0.5) / dWidth;
    float line = min(grid.x, grid.y);

    return 1.0 - smoothstep(0.0, 1.0, line);
}

void main() {
    float minorGrid = drawGrid(vWorldPosition.xz, 1.0);
    float majorGrid = drawGrid(vWorldPosition.xz, 10.0);

    vec4 gridColor = vec4(0.0, 0.0, 0.0, 0.3 * minorGrid);
    gridColor = mix(gridColor, vec4(0.0, 0.0, 0.0, 0.8 * majorGrid), majorGrid);

    vec2 coord = vWorldPosition.xz;
    vec2 dWidth = fwidth(coord);

    if (abs(coord.y) < dWidth.y * 1.5) {
        gridColor.rgb = vec3(0.9, 0.2, 0.2);
        gridColor.a = max(gridColor.a, 0.9);
    }
    if (abs(coord.x) < dWidth.x * 1.5) {
        gridColor.rgb = vec3(0.2, 0.2, 0.9);
        gridColor.a = max(gridColor.a, 0.9);
    }

    float distanceToCamera = distance(vWorldPosition, globalData.uCameraPosition.xyz);
    float fadeLimit = 4800.0;
    float fadeOut = clamp(1.0 - (distanceToCamera / fadeLimit), 0.0, 1.0);
    gridColor.a *= fadeOut;

    if (gridColor.a < 0.01) {
        discard;
    }

    outColor = gridColor;
}
