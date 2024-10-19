#version 410

// Output for shape id
layout(location = 0) out int shape_id;

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
    vec4 color_left[2];
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

//The type of the shape we are rasterizing, the same as the enumerator in shapes.h
// 0 - circles
// 1 - lines
// 2 - BezierCurves (Unused)
uniform uint shape_type;

//The maximum distance to the shape for a pixel to be part of the shape
uniform float rasterize_width;

void main()
{   
    shape_id = -1;

    // ---- CIRCLE
    if (shape_type == 0) {
        vec2 pixel_center = gl_FragCoord.xy;

        for (int i = 0; i < circle_count; ++i) {
            Circle circle = circles[i];
            float distance = length(pixel_center - circle.position);

            // rasterization range: [circle.radius - rasterize_width, circle.radius + rasterize_width]
            if (distance >= (circle.radius - rasterize_width) && distance <= (circle.radius + rasterize_width)) {
                shape_id = i;
                break;
            }
        }
    }
    // ---- LINE
    else if (shape_type == 1) {
        vec2 pixel_center = gl_FragCoord.xy;

        for (int i = 0; i < line_count; ++i) {
            Line line = lines[i];
            vec2 start = line.start_point;
            vec2 end = line.end_point;
        
            // Distance from pixel to the line's start and end points
            float dist_to_start = length(pixel_center - start);
            float dist_to_end = length(pixel_center - end);

            // Check if pixel is within the round cap at the start or end of the line
            if (dist_to_start <= rasterize_width || dist_to_end <= rasterize_width) {
                shape_id = i;
                break;
            }

            // Vector along the line and from start to pixel
            vec2 line_vec = end - start;
            vec2 pixel_vec = pixel_center - start;

            // Projection of pixel_vec onto line_vec to get the closest point on the line
            float line_length = length(line_vec);
            vec2 line_direction = normalize(line_vec);
            float t = dot(pixel_vec, line_direction);  // t = |pixel_vec| cos <pixel_vec, line_vec>

            // Check if the projection falls within the bounds of the line segment
            if (t >= 0.0 && t <= line_length) {
                // Closest point on the line segment to the pixel
                vec2 closest_point = start + t * line_direction;

                // Distance from the pixel to the closest point on the line
                float dist_to_line = length(pixel_center - closest_point);

                // Check if the pixel is within rasterize_width of the line segment
                if (dist_to_line <= rasterize_width) {
                    shape_id = i;
                    break;
                }
            }
        }
    }

}