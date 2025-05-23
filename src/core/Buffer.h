#include <utility>
#pragma once
#include <GL/glew.h>

class VBO {
public:
    VBO()                          { glGenBuffers(1, &id_); }
    ~VBO()                         { glDeleteBuffers(1, &id_); }
    VBO(const VBO&) = delete; VBO& operator=(const VBO&) = delete;
    VBO(VBO&& o) noexcept          { id_ = std::exchange(o.id_, 0); }
    void bind(GLenum target) const { glBindBuffer(target, id_); }
private:
    GLuint id_{};
};

