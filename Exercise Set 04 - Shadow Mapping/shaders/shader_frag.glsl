#version 410 core

// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D texShadow;

// scene uniforms
uniform mat4 lightMVP;
uniform vec3 lightPos;
// config uniforms, use these to control the shader from UI
uniform int samplingMode = 0;
uniform int peelingMode = 0;
uniform int lightMode = 0;
uniform int lightColorMode = 0;


// Bias for shadow comparison to avoid self-shadowing
const float bias = 0.0001;


// Output for on-screen color.
out vec4 outColor;

// Interpolated output data from vertex shader.
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

float shadowTest() {
    // Transform the fragment position to light space
    vec4 fragLightCoord = lightMVP * vec4(fragPos, 1.0);

    // Divide by w because fragLightCoord are homogeneous coordinates
    fragLightCoord.xyz /= fragLightCoord.w;

    // Transform fragLightCoord from NDC space (-1 to 1) to texture space (0 to 1)
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;

    // Depth of the fragment with respect to the light
    float fragLightDepth = fragLightCoord.z;

    // Shadow map coordinate corresponding to this fragment
    vec2 shadowMapCoord = fragLightCoord.xy;

    // Shadow map value from the corresponding shadow map position
    float shadowMapDepth = texture(texShadow, shadowMapCoord).x;

    // Perform the shadow comparison: 
    // if the fragment's depth is greater than the shadow map depth (plus bias), it's in shadow
    return fragLightDepth - bias > shadowMapDepth ? 0 : 1.0; // Returns 0.5 for shadow, 1.0 for light
}

void main()
{
    // Output the normal as color.
    vec3 lightDir = normalize(lightPos - fragPos);

    
    float shadowFactor = shadowTest();
    

    outColor = vec4(vec3(shadowFactor * max(dot(fragNormal, lightDir), 0.0)), 1.0);
}