#version 410

// Global variables for lighting calculations
uniform vec3 lightPos;  // Position of the light source
uniform vec3 lightColor; // Color of the light
uniform vec3 kd; // Diffuse reflectivity

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 N = normalize(fragNormal);  // normal direction
    vec3 L = normalize(lightPos - fragPos);  // light direction

    vec3 diffuse = kd * max(dot(N, L), 0.0) * lightColor;

    outColor = vec4(diffuse, 1.0);
}