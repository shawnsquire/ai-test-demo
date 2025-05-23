#include "Model.h"
void Model::draw() const {
    for (auto& m : meshes) m.draw();
}

