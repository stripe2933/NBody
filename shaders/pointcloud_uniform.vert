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

uniform vec4 body_color;

void main(){
    color = body_color;

    gl_Position = projection_view * vec4(aPosition, 1.0);
    gl_PointSize = mass_factor * aMass + mass_constant;
}