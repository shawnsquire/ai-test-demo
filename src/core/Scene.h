#pragma once
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "Camera.h"
#include "Shader.h"
#include "Model.h"

struct Light {
    glm::vec3 pos;
    glm::vec3 color;
};

class Scene {
public:
    Scene(Shader shader);
    bool load();                 // compile shader + load models
    void draw(int width, int height);
    void update(float dt = 0.016f);
    void onKey(int key);
    void loadModels();


private:
    // camera + misc
    Camera camera_;
    float  camRadius_   = 8.f;
    float  camAngle_    = 0.f;
    bool   autoRotate_  = true;
    bool   showAll_     = false;

    // rendering
    Shader shader_;                  // subsurface shader
    std::vector<Model> models_;
    size_t currentModel_ = 0;

    // lighting / materials
    std::array<Light,4> lights_;
    std::array<SSSMaterial,4> mats_;
    int currentMat_ = 0;

    // helpers
    void   loadMaterials();
    void   setShaderGlobals(int w,int h);
    void   drawModel(const Model& m) const;
};

