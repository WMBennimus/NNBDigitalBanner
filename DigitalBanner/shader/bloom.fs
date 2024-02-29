#version 330 core
out vec4 FragColor;
in vec2 TC;

uniform sampler2D bb;
uniform bool horizontal;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main(){
    vec2 toffs = 1.0 / textureSize(bb, 0);
    vec3 result = texture(bb, TC).rgb * weight[0];
    if(horizontal) for(int i = 1; i < 5; i++){
        result += texture(bb, TC+vec2(toffs.x*i, 0.0)).rgb*weight[i];
        result += texture(bb, TC-vec2(toffs.x*i, 0.0)).rgb*weight[i];
    }else for(int i = 1; i < 5; i++) {
        result += texture(bb, TC+vec2(0.0, toffs.y*i)).rgb*weight[i];
        result += texture(bb, TC-vec2(0.0, toffs.y*i)).rgb*weight[i];
    }
    FragColor = vec4(result, 1.0);
}
