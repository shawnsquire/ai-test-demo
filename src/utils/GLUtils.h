#pragma once
#include <GL/glew.h>
inline void GLcheck(const char* msg = "") {
    while (GLenum e = glGetError()) {
        // Optional: print e
    }
}
