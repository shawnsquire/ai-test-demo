#include "Shader.h"
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <utility>

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    unsigned vs = compile(loadFile(vertPath), GL_VERTEX_SHADER);
    unsigned fs = compile(loadFile(fragPath), GL_FRAGMENT_SHADER);

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);

    int ok;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(program_, 512, nullptr, buf);
        std::cerr << "Link error: " << buf << '\n';
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader()           { glDeleteProgram(program_); }
Shader::Shader(Shader&& o) noexcept { program_ = std::exchange(o.program_, 0); }
Shader& Shader::operator=(Shader&& o) noexcept {
    glDeleteProgram(program_);
    program_ = std::exchange(o.program_, 0);
    return *this;
}

void Shader::use() const { glUseProgram(program_); }

void Shader::set(const std::string& n,int v)    const{ glUniform1i(glGetUniformLocation(program_,n.c_str()),v); }
void Shader::set(const std::string& n,float v)  const{ glUniform1f(glGetUniformLocation(program_,n.c_str()),v); }
void Shader::set(const std::string& n,const glm::vec3& v) const{ glUniform3fv(glGetUniformLocation(program_,n.c_str()),1,&v[0]); }
void Shader::set(const std::string& n,const glm::mat4& v) const{ glUniformMatrix4fv(glGetUniformLocation(program_,n.c_str()),1,GL_FALSE,&v[0][0]); }
void Shader::set(const std::string& n,const glm::vec3* v,int cnt) const{
    glUniform3fv(glGetUniformLocation(program_,n.c_str()),cnt,&v[0].x);
}

unsigned Shader::compile(const std::string& src,unsigned type){
    unsigned s=glCreateShader(type);
    const char* csrc=src.c_str();
    glShaderSource(s,1,&csrc,nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){char buf[512];glGetShaderInfoLog(s,512,nullptr,buf);std::cerr<<"Compile error: "<<buf<<'\n';}
    return s;
}
std::string Shader::loadFile(const std::string& p){
    std::ifstream f(p); std::stringstream ss; ss<<f.rdbuf(); return ss.str();
}

