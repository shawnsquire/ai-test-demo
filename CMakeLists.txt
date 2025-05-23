cmake_minimum_required(VERSION 3.10)
project(SubsurfaceScatteringDemo)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)

add_executable(sss_demo main.cpp)

target_link_libraries(sss_demo 
    ${GLFW_LIBRARIES} 
    ${OPENGL_LIBRARIES} 
    GLEW::GLEW
    assimp
)

target_include_directories(sss_demo PRIVATE ${GLFW_INCLUDE_DIRS})
