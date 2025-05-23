#include "Mesh.h"
#include <GL/glew.h>

Mesh::Mesh(std::vector<Vertex> v,std::vector<unsigned> i)
:verts_(std::move(v)),idx_(std::move(i)){ setup(); }

void Mesh::setup(){
    glGenVertexArrays(1,&vao_);
    vbo_.bind(GL_ARRAY_BUFFER);
    glBindVertexArray(vao_);
    glBufferData(GL_ARRAY_BUFFER,verts_.size()*sizeof(Vertex),verts_.data(),GL_STATIC_DRAW);

    glGenBuffers(1,&ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx_.size()*sizeof(unsigned),idx_.data(),GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,norm));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));

    glBindVertexArray(0);
}
void Mesh::draw() const{
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, idx_.size(), GL_UNSIGNED_INT, 0);
}

