#version 330 core

in vec3 color;
out vec4 FragColor;

void main(){
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float transparency = mix(1.0, 0.0, dot(circCoord, circCoord));
    FragColor = vec4(color, transparency);
}