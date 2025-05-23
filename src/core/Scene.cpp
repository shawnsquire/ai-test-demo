#include "Scene.h"
#include "../loaders/AssimpLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include "Sphere.h"
#include <filesystem>

// ───────────────────────────────────────────────────────── helpers
static constexpr float PI = 3.14159265359f;

// ───────────────────────────────────────────────────────── public
Scene::Scene(Shader shader) : shader_(std::move(shader)) {}
bool Scene::load() {
    // shader – GLSL files live under /assets/shaders/
    std::string root = std::filesystem::current_path().parent_path(); // build → project root
    shader_ = Shader(root + "/assets/shaders/sss.vert", root + "/assets/shaders/sss.frag");
    loadMaterials();
    loadModels();

    // static lights
    lights_ = {{
        {{  0, 6,  12}, {5,4,3.5}},
        {{  0, 4,  -8}, {3.5,4,5}},
        {{  6, 3,   6}, {4,5,4}},
        {{ -6, 3,  -6}, {4.5,4.5,4.5}}
    }};
    return true;
}

void Scene::update(float dt) {
     if (autoRotate_) camAngle_ += dt * 0.3f;
     camera_.orbit(camRadius_, camAngle_, models_[currentModel_].transform[3]);
 }

void Scene::draw(int w, int h) {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_.use();
    setShaderGlobals(w,h);

    if (showAll_) {
        float ring = 8.f;
        for (size_t i = 0; i < models_.size(); ++i) {
            const Model& m = models_[i];
            float ang = (float)i / models_.size() * 2*PI;
            glm::vec3 offset = {cos(ang)*ring, 0, sin(ang)*ring};
            glm::mat4 T = glm::translate(m.transform, offset) * glm::scale(glm::mat4(1), glm::vec3(0.7f));
            shader_.set("model", T);
            drawModel(m);
        }
    } else {
        drawModel(models_[currentModel_]);
    }
}

void Scene::onKey(int key) {
    switch(key){
        case GLFW_KEY_SPACE:
            currentModel_ = (currentModel_+1)%models_.size();
            break;
        case GLFW_KEY_TAB:
            showAll_ = !showAll_;
            break;
        case GLFW_KEY_M:
            currentMat_ = (currentMat_+1)%mats_.size();
            break;
        case GLFW_KEY_R:
            autoRotate_ = !autoRotate_;
            break;
    }
}
// ───────────────────────────────────────────────────────── private
void Scene::loadMaterials() {
    mats_ = {{
        {{0.9,0.7,0.5},{0.1,0.3,0.6},{1,0.6,0.4},0.4,0.5,0.4,0.9}, // skin
        {{0.8,0.8,0.9},{0.05,0.05,0.1},{0.9,0.9,1},0.6,0.3,0.2,0.7}, // marble
        {{1,0.9,0.7},{0.2,0.4,0.8},{1,0.8,0.6},0.8,0.7,0.6,0.95},  // wax
        {{0.6,0.9,0.7},{0.3,0.1,0.2},{0.7,1,0.8},0.3,0.4,0.3,0.8}   // jade
    }};
}

void Scene::loadModels() {
    // simple sphere test model
    Model m;
    m.meshes.emplace_back(makeSphere());
    m.transform = glm::mat4(1.0f);
    m.shadowIndex = 0;
    models_.push_back(std::move(m));

    // load external models
    auto addModel=[&](const char* path, glm::vec3 scale, glm::vec3 pos){
        Model m;
        loaders::loadModel(path,m);
        m.transform = glm::translate(glm::mat4(1), pos) *
                      glm::scale(glm::mat4(1), scale);
        models_.push_back(std::move(m));
    };
    addModel("assets/models/bunny.obj", glm::vec3(0.1f,0.1f,0.1f), glm::vec3(0,0,0));
    addModel("assets/models/dragon.obj", glm::vec3(0.1f,0.1f,0.1f), glm::vec3(0,0,0));
    addModel("assets/models/lucy.obj", glm::vec3(0.1f,0.1f,0.1f), glm::vec3(0,0,0));
}

void Scene::setShaderGlobals(int w,int h){
    // camera matrices
    shader_.set("view", camera_.view());
    shader_.set("projection", camera_.proj(static_cast<float>(w)/h));
    shader_.set("camPos", camera_.view()[3]); // eye in world space

    // lights
    shader_.set("materialType", currentMat_);
    for(int i=0;i<lights_.size();++i){
        shader_.set("lightPositions["+std::to_string(i)+"]", lights_[i].pos);
        shader_.set("lightColors["+std::to_string(i)+"]",    lights_[i].color);
    }

    // active material uniforms
    const auto& m = mats_[currentMat_];
    shader_.set("scatteringCoeff", m.scatter);
    shader_.set("absorptionCoeff", m.absorb);
    shader_.set("internalColor",   m.internal);
    shader_.set("scatteringDistance", m.dist);
    shader_.set("thickness", m.thick);
    shader_.set("roughness", m.rough);
    shader_.set("subsurfaceMix", m.mix);
}

void Scene::drawModel(const Model& m) const {
    shader_.set("model", m.transform);
    for(auto& mesh : m.meshes) mesh.draw();
}

