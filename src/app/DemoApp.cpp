#include "DemoApp.h"
#include <GL/glew.h>
#include "utils/GLUtils.h"
#include <iostream>
#include <memory>

DemoApp::DemoApp() = default;

bool DemoApp::init(int w,int h){
    if(!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    win_=glfwCreateWindow(w,h,"Sexy SSS",nullptr,nullptr);
    if(!win_) return false;
    glfwMakeContextCurrent(win_);
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed\n";
        return false;
    }

    scene_ = std::make_unique<Scene>(
        Shader("assets/shaders/sss.vert", "assets/shaders/sss.frag")
    );


    if (glewInit() != GLEW_OK) return false;
    glfwSetWindowUserPointer(win_,this);
    glfwSetKeyCallback(win_,key);
    width_=w;height_=h;
    scene_->loadModels();
    return true;
}
void DemoApp::run(){
    while(!glfwWindowShouldClose(win_)){
        scene_->update();
        scene_->draw(width_, height_);
        glfwSwapBuffers(win_);
        glfwPollEvents();
    }
}
void DemoApp::key(GLFWwindow* w, int key, int scancode, int action, int mods){
    if (action != GLFW_PRESS) return;
    auto* app = static_cast<DemoApp*>(glfwGetWindowUserPointer(w));
    app->scene_->onKey(key);
}

