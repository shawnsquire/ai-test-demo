#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
void Camera::orbit(float r,float a,const glm::vec3& t){
    target_=t;
    pos_   = t + glm::vec3(sin(a)*r, 2.0f, cos(a)*r);
}
glm::mat4 Camera::view() const { return glm::lookAt(pos_, target_, up_); }
glm::mat4 Camera::proj(float aspect) const { return glm::perspective(glm::radians(fov_), aspect, .1f, 100.f); }

