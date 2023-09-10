#version 330 core

layout(location = 0) in float aMass;
layout(location = 1) in vec3 aPosition;
layout(location = 2) in vec3 aVelocity;

out vec3 color;

layout (std140) uniform ProjectionView{
    mat4 projection_view;
};

layout (std140) uniform PointSize{
    float mass_factor;
    float mass_constant;
};

void main(){
    color = vec3(0.2, 0.5, 1.0);

    gl_Position = projection_view * vec4(aPosition, 1.0);
    gl_PointSize = mass_factor * aMass + mass_constant;
}