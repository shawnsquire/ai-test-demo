#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Buffer.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
};

class Mesh {
public:
    Mesh(std::vector<Vertex> v, std::vector<unsigned> i);
    void draw() const;

private:
    std::vector<Vertex>   verts_;
    std::vector<unsigned> idx_;
    GLuint vao_{}, ebo_{};
    VBO    vbo_;
    void   setup();
};

