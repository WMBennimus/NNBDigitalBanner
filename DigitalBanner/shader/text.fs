#version 330 core
in vec2 TC;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;

void main(){
    float t = texture(text, TC).r;
    if(t < 0.9) discard;
    FragColor = vec4(textColor,1.0);
}