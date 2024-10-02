#version 410 core

// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D texShadow;

// scene uniforms
uniform mat4 lightMVP;
uniform vec3 lightPos;
// config uniforms, use these to control the shader from UI
uniform int samplingMode;
uniform int peelingMode = 0;
uniform int lightMode = 0;
uniform int lightColorMode = 0;


// Bias for shadow comparison to avoid self-shadowing
const float bias = 0.0005;
// Spotlight dimming power
const float spotlightPower = 0.5;  // 1: smooth boundary, 0.1: sharp boundary


// Output for on-screen color.
out vec4 outColor;

// Interpolated output data from vertex shader.
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal


// calculate spotlight dimming based on distance from shadow map center
float spotlightDimming(vec2 shadowMapCoord) {
    
    float distanceFromCenter = distance(shadowMapCoord, vec2(0.5));

    // If the distance is greater than 0.5, the light should be fully dimmed (outside spotlight)
    if (distanceFromCenter >= 0.5) return 0.0;

    // Otherwise, calculate the dimming factor, scaling the distance and raising to the power
    float dimmingFactor = pow(1.0 - (distanceFromCenter / 0.5), spotlightPower);
    return dimmingFactor;
}


// calculate the shadow factor
float shadowTest() {
    // Transform the fragment position to light space
    vec4 fragLightCoord = lightMVP * vec4(fragPos, 1.0);

    // Divide by w because fragLightCoord are homogeneous coordinates
    fragLightCoord.xyz /= fragLightCoord.w;

    // Transform fragLightCoord from NDC space (-1 to 1) to texture space (0 to 1)
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;

    // Depth of the fragment with respect to the light
    float currentDepth = fragLightCoord.z;

    // Shadow map coordinate corresponding to this fragment
    vec2 shadowMapCoord = fragLightCoord.xy;

    // Handle spotlight dimming based on distance from shadow map center
    float dimmingFactor = spotlightDimming(shadowMapCoord);

    if (samplingMode == 0) {
        // Shadow map value from the corresponding shadow map position
        float shadowMapDepth = texture(texShadow, shadowMapCoord).x;
        // if the fragment's depth is greater than the shadow map depth (plus bias), it's in shadow
        float shadowFactor = currentDepth - bias > shadowMapDepth ? 0 : 1.0;
        return shadowFactor * dimmingFactor;
    } else {  // PCF
        float shadowFactor = 0.0;
        vec2 texelSize = 1.0 / textureSize(texShadow, 0);
        for(int x = -2; x <= 2; ++x) {
            for(int y = -2; y <= 2; ++y) {
                vec2 offset = vec2(x, y) * texelSize;
                float sampleShadowMapDepth = texture(texShadow, shadowMapCoord + offset).x; 
                shadowFactor += currentDepth - bias > sampleShadowMapDepth ? 0.0 : 1.0;        
            }    
        }
        shadowFactor /= 25.0;  // number of samples: 25
        return shadowFactor * dimmingFactor;
    }
}

void main()
{
    // Output the normal as color.
    vec3 lightDir = normalize(lightPos - fragPos);

    
    float shadowFactor = shadowTest();
    

    outColor = vec4(vec3(shadowFactor * max(dot(fragNormal, lightDir), 0.0)), 1.0);
}