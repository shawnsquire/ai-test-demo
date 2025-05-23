#pragma once
#include <vector>
#include "core/Scene.h"
#include <GLFW/glfw3.h>
#include <memory>

class DemoApp {
public:
    DemoApp();
    bool init(int w, int h);
    void run();
private:
    GLFWwindow* win_ = nullptr;
    int width_ = 1400;
    int height_ = 900;

    static void key(GLFWwindow* w, int key, int scancode, int action, int mods);
    std::unique_ptr<Scene> scene_;
};

