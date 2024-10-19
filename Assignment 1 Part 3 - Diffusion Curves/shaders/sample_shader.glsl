#version 410

#define M_PI 3.14159265359f
#define EPSILON 0.00001f

// Output for accumulated color
layout(location = 0) out vec4 outColor;

//Set gl_FragCoord to pixel center
layout(pixel_center_integer) in vec4 gl_FragCoord;


//Circle and line struct equivalent to the one in shape.h
struct Circle {
    vec4 color;
    vec2 position;
    float radius;
};

struct Line {
    vec2 start_point;
    vec2 end_point;
    vec4 color_left[2];  // start and end point left color and right color
    vec4 color_right[2];
};

//The uniform buffers for the shapes containing the count of shapes in the first slot and after that, the actual shapes
//We need to give the arrays a fixed size to work with opengl 4.1
layout(std140) uniform circleBuffer
{
    int circle_count;
    Circle circles[32];
};

layout(std140) uniform lineBuffer
{
    int line_count;
    Line lines[800];
};

//Textures for the rasterized shapes, and the accumulator
uniform isampler2D rasterized_texture;
uniform sampler2D accumulator_texture;

//The type of the shape we are rasterizing, the same as the enumerator in shapes.h
// 0 - circles
// 1 - lines
// 2 - BezierCurves (Unused)
uniform uint shape_type;

//The number of samples we have already taken
uniform uint frame_nr;
//Screen dimensions
uniform ivec2 screen_dimensions;

//Step size for ray-marching
uniform float step_size; 

//The maximum amount of raymarching steps we can take
uniform uint max_raymarch_iter;

//Random number generator outputs numbers between [0-1]
float get_random_numbers(inout uint seed) {
    seed = 1664525u * seed + 1013904223u;
    seed += 1664525u * seed;
    seed ^= (seed >> 16u);
    seed += 1664525u * seed;
    seed ^= (seed >> 16u);
    return seed * pow(0.5, 32.0);
}
vec2 march_ray(vec2 origin, vec2 direction, float step_size) {
    vec2 current_position = origin;

    for (int i = 0; i < max_raymarch_iter; ++i) {
        // screen space position to texture coordinates
        vec2 tex_corrds = current_position / vec2(screen_dimensions);

        if (tex_corrds.x < 0.0 || tex_corrds.x > 1.0 || 
            tex_corrds.y < 0.0 || tex_corrds.y > 1.0) {
            break;
        }
        
        // texelFetch: retrieves a texel directly by integer pixel coordinates (no interpolation)
        int shape_index = texelFetch(rasterized_texture, ivec2(current_position), 0).r;
        // texture: samples a texture using normalized coordinates (interpolation)
        //int shape_index = texture(rasterized_texture, tex_corrds).x;

        if (shape_index >= 0) return current_position;  // return if hit

        current_position += direction * step_size;
    }

    return origin;
}

void main()
{
    //If a shape is hit we can sample it
    bool hit = false;

    uint seed = uint(gl_FragCoord.x) * 2973u + uint(gl_FragCoord.y) * 3277u + uint(frame_nr) * 2699u;
    float random_angle = get_random_numbers(seed) * 2.0 * M_PI;  // [0, 2PI]
    vec2 direction = vec2(cos(random_angle), sin(random_angle));

    vec2 pixel_center = gl_FragCoord.xy;
    vec2 intersection = march_ray(pixel_center, direction, step_size);
    vec4 accumulated_color = vec4(0.0);

    int shape_index = texelFetch(rasterized_texture, ivec2(intersection), 0).r;
    //vec2 tex_corrds = intersection / vec2(screen_dimensions);
    //int shape_index = texture(rasterized_texture, tex_corrds).x;

    if (shape_index >= 0) {
        if (shape_type == 0) {  // Circle
            Circle circle = circles[shape_index];
            float distance2center = distance(pixel_center, circle.position);

            if (distance2center <= circle.radius) {
                accumulated_color += circle.color;
                hit = true;
            }
        } else {  // line
            Line line = lines[shape_index];
            vec2 start = line.start_point;
            vec2 end = line.end_point;

            // Determine which side of the line is hit
            vec2 line_direction = normalize(end - start);
            vec2 pixel_vec = pixel_center - start;
            float t = dot(pixel_vec, line_direction);
            float cross_product = line_direction.x * pixel_vec.y - line_direction.y * pixel_vec.x;

            // Select color based on which side the intersection is on
            float colorRatio = clamp(t, 0.0, 1.0);
            vec4 blended_color;
            if (cross_product > 0.0) {
                // Left side hit
                //blended_color = line.color_left[0];
                blended_color = mix(line.color_left[0], line.color_left[1], colorRatio);
            } else {
                // Right side hit
                //blended_color = line.color_right[0];
                blended_color = mix(line.color_right[0], line.color_right[1], colorRatio);
            }

            // Calculate the distance between the pixel center and the intersection
            float distance_to_intersection = distance(pixel_center, intersection);
            float weight = 1.0 / (distance_to_intersection + EPSILON);  // Add epsilon to avoid division by zero

            // Accumulate the weighted color
            accumulated_color += blended_color * weight;
            hit = true;
        }
        
    }

    vec4 previous_color = texelFetch(accumulator_texture, ivec2(pixel_center), 0);
    //vec4 previous_color = texture(accumulator_texture, pixel_center / screen_dimensions);

    if(hit){
        // ---- Circle
        if (shape_type == 0) {
            outColor = previous_color + accumulated_color;
            // FIXED: Removed code below. Weighted average is computed in color_shader.glsl
            // outColor = (accumulated_color + previous_color * float(frame_nr)) / (float(frame_nr) + 1.0);
        }
        // ---- Line
        else if (shape_type == 1) {
            outColor = previous_color + accumulated_color;
        }
    } else {
        outColor = previous_color;
    }
    // outColor = accumulated_color;
}
