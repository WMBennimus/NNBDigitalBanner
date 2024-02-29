#version 330 core
out vec4 FragColor;
in vec2 TC;

uniform sampler2D tex;

void main(){
    vec4 t = texture(tex,TC);
    if(t.a < 0.8) discard;
    FragColor = t;
}
