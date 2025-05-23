#version 330 core
in vec3 vPos, vNormal;
out vec4 FragColor;

uniform vec3 camPos;
uniform vec3 scatteringCoeff, absorptionCoeff, internalColor;
uniform float scatteringDistance, thickness, roughness, subsurfaceMix;
uniform vec3 lightPositions[4], lightColors[4];
uniform int  materialType;

void main(){
    vec3 N = normalize(vNormal);
    vec3 V = normalize(camPos - vPos);
    vec3 color = vec3(0.1);                // quick fake SSS – replace later

    for(int i=0;i<4;++i){
        vec3 L = normalize(lightPositions[i]-vPos);
        float d = length(lightPositions[i]-vPos);
        float atten = 1.0/(d*d+1.0);
        color += lightColors[i] * max(dot(N,L),0.0) * atten;
    }
    FragColor = vec4(pow(color, vec3(1.0/2.2)),1.0);
}

