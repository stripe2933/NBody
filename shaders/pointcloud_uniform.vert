#version 330 core

layout(location = 0) in float aMass;
layout(location = 1) in vec3 aPosition;
layout(location = 2) in vec3 aVelocity;

out vec3 color;

uniform mat4 projection_view;
uniform float mass_factor;
uniform float mass_constant;

void main(){
    color = vec3(0.2, 0.5, 1.0);

    gl_Position = projection_view * vec4(aPosition, 1.0);
    gl_PointSize = mass_factor * aMass + mass_constant;
}