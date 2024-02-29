#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
uniform sampler2D diff;
uniform sampler2D norm;
uniform sampler2D smap;
in vec2 TC;
in vec3 FP;

uniform vec3 uRL;
uniform vec3 uGL;
uniform vec3 uBL;
uniform vec3 uWL;
uniform vec3 red;
uniform vec3 green;
uniform vec3 blue;

float specular_intensity = 1.5;
vec3 ambience = vec3(0.1,0.1,0.1);
int power = 8;
float bloomThreshold = 0.9;

void main(){
    vec3 nm = texture(norm, TC).rgb;
    nm = normalize((nm - vec3(0.5,0.5,0.5))*2.0);
    vec3 redDir = normalize(uRL - FP);
    vec3 redRef = reflect(-redDir,nm);
    vec3 greenDir = normalize(uGL - FP);
    vec3 greenRef = reflect(-greenDir,nm);
    vec3 blueDir = normalize(uBL - FP);
    vec3 blueRef = reflect(-blueDir,nm);
    float sR = pow(max(dot(redDir,redRef),0.0),power);
    float sG = pow(max(dot(greenDir,greenRef),0.0),power);
    float sB = pow(max(dot(blueDir,blueRef),0.0),power);
    vec3 spec = specular_intensity * vec3(sR + sG + sB);
    vec3 rdiff = max(dot(nm, redDir),0.0)*red;
    vec3 gdiff = max(dot(nm, greenDir),0.0)*green;
    vec3 bdiff = max(dot(nm, blueDir),0.0)*blue;
    FragColor = vec4((ambience + rdiff + gdiff + bdiff),1.0) * texture(diff, TC) + vec4(spec, 1.0) * texture(smap,TC);
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > bloomThreshold) BrightColor = vec4(FragColor.rgb, 1.0);
    else BrightColor = vec4(0.0,0.0,0.0,1.0);
}
