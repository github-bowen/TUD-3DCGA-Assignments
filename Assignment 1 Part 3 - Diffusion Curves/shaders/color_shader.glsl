#version 410

// Output for on-screen color
layout(location = 0) out vec4 outColor;

//Textures
uniform sampler2D accumulator_texture;
uniform ivec2 screen_dimensions;

//Set gl_FragCoord to pixel center
layout(pixel_center_integer) in vec4 gl_FragCoord;

void main()
{
    //vec2 texCoords = gl_FragCoord.xy / screen_dimensions;
    //vec4 accumulated_color = texture(accumulator_texture, texCoords);
    vec4 accumulated_color = texelFetch(accumulator_texture, ivec2(gl_FragCoord.xy), 0);

    float sample_count = accumulated_color.a;

    vec3 average_color = sample_count > 0.0 ? accumulated_color.rgb / sample_count : vec3(0.0);

    // Output the final color with full opacity
    outColor = vec4(average_color, 1.0);
}
