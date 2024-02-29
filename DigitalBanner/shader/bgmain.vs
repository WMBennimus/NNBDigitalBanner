#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTC;

out vec2 TC;
out vec3 FP;

uniform mat4 uPM;

void main(){
    gl_Position = uPM*vec4(aPos, 1.0);
    TC = aTC;
    FP = aPos;
}
