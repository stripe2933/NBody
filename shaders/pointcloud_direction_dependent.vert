#version 330 core

#define M_PI 3.1415926535897932384626433832795

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

uniform float offset;

// https://gist.github.com/983/e170a24ae8eba2cd174f
vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main(){
    float angle = atan(aPosition.z, aPosition.x) + offset;
    float hue = fract((angle - M_PI) / (2.0 * M_PI));
    color = vec4(hsv2rgb(vec3(hue, 1.0, 1.0)), 1.0);

    gl_Position = projection_view * vec4(aPosition, 1.0);
    gl_PointSize = mass_factor * aMass + mass_constant;
}