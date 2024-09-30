#version 410

// Global variables for lighting calculations
uniform vec3 lightPos;  // Position of the light source
// uniform vec3 lightColor; // Color of the light  FIXME: deleted
// uniform vec3 ks; // Specular reflectivity  FIXME: deleted
uniform float shininess; // Shininess exponent
uniform vec3 cameraPos;  // Position of the camera

uniform float toonSpecularThreshold; // Threshold for toon specular highlights

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPos);
    vec3 V = normalize(cameraPos - fragPos);

    vec3 H = normalize(L + V);  // halfway vector H

    // vec3 specular = ks * pow(max(dot(H, N), 0.0), shininess) * lightColor;
    float specularFactor = pow(max(dot(H, N), 0.0), shininess);

    outColor = (specularFactor >= toonSpecularThreshold) ? vec4(1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}