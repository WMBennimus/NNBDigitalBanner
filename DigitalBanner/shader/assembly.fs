#version 330 core
out vec4 FragColor;
in vec2 TC;

uniform sampler2D frag;
uniform sampler2D bloom;
uniform float exposure;

void main(){
    const float gamma = 2.2;
    vec3 diff = texture(frag, TC).rgb;
    vec3 blm = texture(bloom, TC).rgb;
    diff += blm;
    FragColor = vec4(diff, 1.0);
}