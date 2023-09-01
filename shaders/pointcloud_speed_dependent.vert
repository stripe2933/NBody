#version 330 core

layout(location = 0) in float aMass;
layout(location = 1) in vec3 aPosition;
layout(location = 2) in vec3 aVelocity;

out vec3 color;

uniform mat4 projection_view;
uniform float mass_factor;
uniform float mass_constant;
uniform float speed_low;
uniform float speed_high;
uniform vec3 color_low;
uniform vec3 color_high;

void main(){
    color = mix(color_low, color_high, (length(aVelocity) - speed_low) / (speed_high - speed_low));

    gl_Position = projection_view * vec4(aPosition, 1.0);
    gl_PointSize = mass_factor * aMass + mass_constant;
}