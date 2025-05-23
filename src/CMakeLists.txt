file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
     core/*.cpp loaders/*.cpp app/*.cpp)

add_executable(sexy_sss ${SRC})

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW   REQUIRED)
find_package(glm    REQUIRED)
find_package(assimp REQUIRED)

target_include_directories(sexy_sss PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/loaders
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(sexy_sss
    ${OPENGL_LIBRARIES}
    glfw
    GLEW
    assimp
)

