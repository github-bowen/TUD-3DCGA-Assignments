#version 430

in vec3 fragNormal;
in vec4 fragPosition;

uniform vec3 planeColor;
uniform float opacity;
uniform int peelingLayer;
uniform sampler2D prevDepthTexture;

layout(location = 0) out vec4 fragColor;

void main() {
    // For first layer, just output color with alpha
    if (peelingLayer == 0) {
        fragColor = vec4(planeColor, opacity);
        return;
    }
    
    // For subsequent layers, check against previous depth
    float prevDepth = texture(prevDepthTexture, gl_FragCoord.xy / textureSize(prevDepthTexture, 0)).r;
    if (gl_FragCoord.z <= prevDepth) {
        discard;
    }
    
    fragColor = vec4(planeColor, opacity);
}