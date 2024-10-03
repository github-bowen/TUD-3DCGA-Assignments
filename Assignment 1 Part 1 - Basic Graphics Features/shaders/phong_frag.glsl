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

// NOTE: Shadow shader below


uniform sampler2D texShadow;
uniform sampler2D texLight;

uniform mat4 lightMVP;

uniform int shadow;  // 0: no shadow
uniform int pcf;  // 0: Single Sample, 1: PCF
uniform int peelingMode = 0;  // 1: peeling mode
uniform int lightMode;  // 0: Normal, 1: Spotlight
uniform int lightColorMode;  // 0: White, 1: Textured

// Bias for shadow comparison to avoid self-shadowing
const float bias = 0.0005;
// Spotlight dimming power
const float spotlightPower = 0.5;  // 1: smooth boundary, 0.1: sharp boundary


// calculate spotlight dimming based on distance from shadow map center
float spotlightDimming(vec2 shadowMapCoord) {

    if (lightMode == 0) return 1.0;  // normal mode
    
    float distanceFromCenter = distance(shadowMapCoord, vec2(0.5));

    // If the distance is greater than 0.5, the light should be fully dimmed (outside spotlight)
    if (distanceFromCenter >= 0.5) return 0.0;

    // Otherwise, calculate the dimming factor, scaling the distance and raising to the power
    float dimmingFactor = pow(1.0 - (distanceFromCenter / 0.5), spotlightPower);  // 0.5 - distanceFromCenter;
    return dimmingFactor;
}


// calculate the shadow factor
float shadowTest(vec2 shadowMapCoord, float currentDepth) {

    if (shadow == 0) return 1.0;
    if (pcf == 0) {
        // Shadow map value from the corresponding shadow map position
        float shadowMapDepth = texture(texShadow, shadowMapCoord).x;
        // if the fragment's depth is greater than the shadow map depth (plus bias), it's in shadow
        float shadowFactor = currentDepth - bias > shadowMapDepth ? 0 : 1.0;
        return shadowFactor;
    } else {  // PCF
        float shadowFactor = 0.0;
        vec2 texelSize = 1.0 / textureSize(texShadow, 0);
        for(int x = -1; x <= 1; ++x) {
            for(int y = -1; y <= 1; ++y) {
                vec2 offset = vec2(x, y) * texelSize;
                float sampleShadowMapDepth = texture(texShadow, shadowMapCoord + offset).x; 
                shadowFactor += currentDepth - bias > sampleShadowMapDepth ? 0.0 : 1.0;        
            }    
        }
        shadowFactor /= 9.0;  // number of samples: 9
        return shadowFactor;
    }
}

struct ShadowInfo {
    vec3 lightColorFactor;
    float dimmingFactor;
    float shadowFactor;
};

ShadowInfo getShadowInfo()
{
    // Output the normal as color.
    vec3 lightDir = normalize(lightPos - fragPos);

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

    // NOTE: calculate different factors
    // Handle spotlight dimming based on distance from shadow map center
    float dimmingFactor = spotlightDimming(shadowMapCoord);

    float shadowFactor = shadowTest(shadowMapCoord, currentDepth);

    // Sample the light color from the light texture using the shadow map coordinates
    vec3 lightColorFactor = (lightColorMode == 1) ? texture(texLight, shadowMapCoord).rgb : vec3(1.0);

    if (peelingMode == 1 && shadowFactor > 0.9) {  // optional: distance(shadowMapCoord, vec2(0.5)) < 0.5
        discard;
    }

    if (peelingMode == 1) {  // for those occluded points, should be bright
        shadowFactor = 1.0;
        dimmingFactor = 1.0;
    }
    // return dimmingFactor * shadowFactor;

    return ShadowInfo( lightColorFactor, dimmingFactor, shadowFactor );

    // outColor = vec4(vec3(lightColorFactor* dimmingFactor * shadowFactor * max(dot(fragNormal, lightDir), 0.0)), 1.0);
}

// NOTE: Shadow Shader above

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPos);
    vec3 V = normalize(cameraPos - fragPos);

    vec3 R = reflect(-L, N);  // reflection direction

    vec3 specular = ks * pow(max(dot(R, V), 0.0), shininess) * lightColor;

    ShadowInfo shadowInfo = getShadowInfo();
    vec3 lightColorFactor = shadowInfo.lightColorFactor;
    float dimmingFactor = shadowInfo.dimmingFactor;
    float shadowFactor = shadowInfo.shadowFactor;

    outColor = vec4(specular * shadowFactor * dimmingFactor, 1.0);
}