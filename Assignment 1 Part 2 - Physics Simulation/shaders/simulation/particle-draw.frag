#version 410

uniform vec3 containerCenter;

uniform vec3 minSpeedColor;
uniform vec3 maxSpeedColor;
uniform float maxSpeedThreshold;
uniform bool useSpeedBasedColoring;

uniform bool enableShading;
uniform float ambientCoefficient;

uniform bool enableBounce;
// uniform int bounceThreshold;
// uniform int bounceFrames;
uniform vec3 bounceColor;

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

    // ===== Task 3.1 Blinking =====
    int collisionCount = int(fragBounceData.x);
    int frameCounter = int(fragBounceData.y);

    if (enableBounce) {
        if (frameCounter > 0) {
            baseColor = bounceColor;
            // frameCounter--;
        }
    }

    vec3 finalColor = baseColor;

    // ===== Task 2.2 Shading =====

    if (enableShading) {
        // ambient term
        vec3 ambient = ambientCoefficient * finalColor;

        // diffuse term: based on a single light at the center of the container
        vec3 lightDir = normalize(containerCenter - fragPosition);
        float diffuseIntensity = max(0, dot(lightDir, normalize(fragNormal)));
        vec3 diffuse = diffuseIntensity * finalColor;

        finalColor = ambient + diffuse;
    }

    fragColor = vec4(finalColor, 1.0);
}
