#version 330 core

out vec4 FragColor;

uniform vec4 octtree_color;

void main(){
    FragColor = octtree_color;
}