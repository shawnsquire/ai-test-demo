#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    void orbit(float r,float angle,const glm::vec3& tgt={0,0,0});
    glm::mat4 view() const;
    glm::mat4 proj(float aspect) const;

private:
    glm::vec3 pos_{0,2,8}, target_{}, up_{0,1,0};
    float fov_{45.f};
};

