#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 aTC;

out vec2 TC;

uniform float xOffs;

void main(){
    gl_Position = vec4(pos.x+xOffs, -pos.y, 0.0, 1.0);
    TC = aTC;
}
