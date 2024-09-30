#version 410

// Global variables for lighting calculations
uniform vec3 lightPos;  // Position of the light source
uniform vec3 lightColor; // Color of the light
uniform vec3 ks; // Specular reflectivity
uniform float shininess; // Shininess exponent
uniform vec3 cameraPos;  // Position of the camera

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

    vec3 R = reflect(-L, N);  // reflection direction

    vec3 specular = ks * pow(max(dot(R, V), 0.0), shininess) * lightColor;

    outColor = vec4(specular, 1.0);
}