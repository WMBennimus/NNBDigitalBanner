#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 aTC;

out vec2 TC;

uniform float offs;
uniform mat4 rot;
uniform mat4 orth;

void main(){
    gl_Position = rot * vec4(pos, 1.0);
    TC = vec2(0, offs) + aTC;
}
