#version 330 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 aTC;

uniform mat4 proj;

out vec2 TC;

void main(){
    gl_Position = proj*vec4(pos, 0.0, 1.0);
    TC = aTC;
}
