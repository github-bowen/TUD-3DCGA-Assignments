#version 410

uniform vec3 containerCenter;

uniform vec3 minSpeedColor;
uniform vec3 maxSpeedColor;
uniform float maxSpeedThreshold;
uniform bool useSpeedBasedColoring;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragVelocity;
layout(location = 3) in vec3 fragBounceData;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 baseColor = vec3(1.0);

    // ===== Task 2.1 Speed-based Colors =====

    float speed = length(fragVelocity);

    if (useSpeedBasedColoring) {
        float t = clamp(speed / maxSpeedThreshold, 0.0, 1.0);
        baseColor = mix(minSpeedColor, maxSpeedColor, t);  // linear interpolation
    }

    vec3 finalColor = baseColor;

    // ===== Task 2.2 Shading =====

    fragColor = vec4(finalColor, 1.0);
}
