#version 410

// Global variables for lighting calculations
uniform vec3 lightPos;       // Position of the light source
// uniform vec3 lightColor;     // Color of the light  FIXME: deleted
uniform vec3 cameraPos;      // Position of the camera
uniform float shininess; // Shininess exponent
uniform sampler2D texToon;   // The X-Toon texture

// Interpolated output data from vertex shader
in vec3 fragPos;    // World-space position
in vec3 fragNormal; // World-space normal

// Output for on-screen color
out vec4 outColor;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPos);
    vec3 V = normalize(cameraPos - fragPos);

    float diffuseFactor = max(dot(N, L), 0.0);  // Lambertian
    vec3 H = normalize(L + V);
    float specularFactor = pow(max(dot(N, H), 0.0), shininess); // Blinn Phong
    float brightness = 0.5 * diffuseFactor + 0.5 * specularFactor;  // Lambertian + Blinn Phong

    float distance = length(cameraPos - fragPos);
    float scaledDistance = distance / 3.5;

    // limited to between 0 and 1
    brightness = clamp(brightness, 0.0, 1.0);
    scaledDistance = clamp(scaledDistance, 0.0, 1.0);

    outColor = texture(texToon, vec2(brightness, scaledDistance));
    // outColor = (distance >= 3.2) ? vec4(1) : vec4(0);  // DEBUG: find the distance from the viewer to the fragment
}
