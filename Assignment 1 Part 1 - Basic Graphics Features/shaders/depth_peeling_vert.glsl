#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 mvp;
uniform float zOffset;

out vec3 fragNormal;
out vec4 fragPosition;

void main() {
    vec3 offsetPosition = position;
    offsetPosition.z += zOffset;
    gl_Position = mvp * vec4(offsetPosition, 1.0);
    fragPosition = gl_Position;
    fragNormal = normal;
}