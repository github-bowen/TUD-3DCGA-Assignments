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
    }
}