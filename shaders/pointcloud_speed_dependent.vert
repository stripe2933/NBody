#version 330 core

layout(location = 0) in float aMass;
layout(location = 1) in vec3 aPosition;
layout(location = 2) in vec3 aVelocity;

out vec4 color;

layout (std140) uniform ProjectionView{
    mat4 projection_view;
};

layout (std140) uniform PointSize{
    float mass_factor;
    float mass_constant;
};

uniform float speed_low;
uniform float speed_high;
uniform vec4 color_low;
uniform vec4 color_high;

void main(){
    float velocity_percentage = (length(aVelocity) - speed_low) / (speed_high - speed_low);
    color = mix(color_low, color_high, velocity_percentage);

    gl_Position = projection_view * vec4(aPosition, 1.0);
    gl_PointSize = mass_factor * aMass + mass_constant;
}