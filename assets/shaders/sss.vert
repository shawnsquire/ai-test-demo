#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;

uniform mat4 model, view, projection;

out vec3 vPos;
out vec3 vNormal;

void main(){
    vPos     = vec3(model * vec4(aPos,1.0));
    vNormal  = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(vPos,1.0);
}

