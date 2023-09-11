#version 330 core

in vec4 color;
out vec4 FragColor;

void main(){
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float transparency = mix(1.0, 0.0, dot(circCoord, circCoord));
    FragColor = color * vec4(vec3(1.0), transparency);
}