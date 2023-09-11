#version 330 core

layout (location = 0) in vec3 aBasePos;
layout (location = 1) in vec3 aBoxPos;
layout (location = 2) in float aBoxSize;

layout (std140) uniform ProjectionView{
    mat4 projection_view;
};

void main(){
    gl_Position = projection_view * vec4(aBoxPos + aBoxSize * aBasePos, 1.0);
}