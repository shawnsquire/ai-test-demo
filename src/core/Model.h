#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "Mesh.h"

struct SSSMaterial {
    glm::vec3 scatter, absorb, internal;
    float dist, thick, rough, mix;
};

class Model {
public:
    std::vector<Mesh> meshes;
    glm::mat4         transform{1.0f};
    int               materialId{0};
    int shadowIndex = 0;

    void draw() const;
};

