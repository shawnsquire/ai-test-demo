#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>

const float PI = 3.14159265359f;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // Vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // Vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }

    void Draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// Same shaders as before but with better vertex shader
const char* sceneVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
)";

// Updated fragment shader with proper scene lighting
const char* sceneFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoord;

// Mark's SSS Parameters + Disney Model params
uniform vec3 scatteringCoeff;
uniform vec3 absorptionCoeff;
uniform float scatteringDistance;
uniform vec3 internalColor;
uniform float thickness;
uniform float roughness;        // Alpha from the equations
uniform float subsurfaceMix;    // kss from Disney model

uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform vec3 camPos;
uniform float time;

const float PI = 3.14159265359;

// Disney/Burley Subsurface Scattering (from Mark's textbook)
vec3 calculateDisneySubsurface(vec3 L, vec3 N, vec3 V, vec3 lightColor, vec3 albedo) {
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    vec3 H = normalize(L + V);
    float HdotL = max(dot(H, L), 0.0);

    // Ensure we don't divide by zero
    if (NdotL <= 0.0 || NdotV <= 0.0) {
        return vec3(0.0);
    }

    float alpha = roughness * roughness;

    // FD90 calculation from textbook: FD90 = 0.5 + 2√α(h·l)²
    float FD90 = 0.5 + 2.0 * sqrt(alpha) * HdotL * HdotL;

    // Disney diffuse fresnel: fd = (1 + (FD90 - 1)(1 - n·l)^5)(1 + (FD90 - 1)(1 - n·v)^5)
    float fd = (1.0 + (FD90 - 1.0) * pow(1.0 - NdotL, 5.0)) * 
               (1.0 + (FD90 - 1.0) * pow(1.0 - NdotV, 5.0));

    // FSS90 calculation: FSS90 = √α(h·l)²
    float FSS90 = sqrt(alpha) * HdotL * HdotL;

    // FSS fresnel term
    float FSS = (1.0 + (FSS90 - 1.0) * pow(1.0 - NdotL, 5.0)) * 
                (1.0 + (FSS90 - 1.0) * pow(1.0 - NdotV, 5.0));

    // Subsurface term: fss = (1/(n·l)(n·v) - 0.5)FSS + 0.5
    float fss = (1.0 / (NdotL * NdotV) - 0.5) * FSS + 0.5;

    // Disney diffuse BRDF: fdiff(l,v) = χ⁺(n·l)χ⁺(n·v)(ρss/π)((1-kss)fd + 1.25kss*fss)
    // χ⁺ is max(0, x), which we already applied to NdotL and NdotV
    vec3 rho_ss = albedo; // Surface albedo
    float kss = subsurfaceMix; // Subsurface mix factor

    vec3 fdiff = NdotL * NdotV * (rho_ss / PI) * ((1.0 - kss) * fd + 1.25 * kss * fss);

    return fdiff * lightColor;
}

// Enhanced subsurface scattering with proper scattering/absorption
vec3 calculateEnhancedSSS(vec3 L, vec3 N, vec3 V, vec3 lightColor) {
    // Wrap-around lighting for subsurface penetration
    float wrap = scatteringDistance;
    float NdotL = dot(N, L);
    float wrappedDiffuse = max(0.0, (NdotL + wrap) / (1.0 + wrap));

    // Transmission component with proper thickness
    vec3 H = normalize(L + N * scatteringDistance);
    float VdotH = max(0.0, dot(-V, H));
    float transmission = pow(VdotH, 3.0) * thickness;

    // Apply Mark's scattering and absorption coefficients
    vec3 scatteredLight = lightColor * scatteringCoeff * wrappedDiffuse;
    vec3 transmittedLight = lightColor * internalColor * transmission;

    // Beer's law for absorption with proper distance scaling
    vec3 attenuation = exp(-absorptionCoeff * scatteringDistance * 3.0);

    return (scatteredLight + transmittedLight) * attenuation;
}

// Improved scene GI
vec3 calculateSceneGI(vec3 worldPos, vec3 normal) {
    vec3 ambient = vec3(0.12, 0.12, 0.15);

    // Sky hemisphere
    float skyFactor = max(0.0, dot(normal, vec3(0, 1, 0)));
    vec3 skyContribution = vec3(0.4, 0.6, 1.0) * skyFactor * 0.25;

    // Ground bounce
    float groundFactor = max(0.0, dot(normal, vec3(0, -1, 0)));
    vec3 groundContribution = vec3(0.8, 0.6, 0.4) * groundFactor * 0.08;

    return ambient + skyContribution + groundContribution;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    // Scene global illumination
    vec3 globalIllum = calculateSceneGI(WorldPos, N);

    // Material properties
    vec3 albedo = vec3(0.7, 0.5, 0.4); // Skin-like base color

    vec3 totalLighting = vec3(0.0);

    for(int i = 0; i < 4; ++i) {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance + 1.0);
        vec3 radiance = lightColors[i] * attenuation;

        // Disney subsurface scattering (from Mark's textbook)
        vec3 disneySSS = calculateDisneySubsurface(L, N, V, radiance, albedo);

        // Enhanced SSS with Mark's parameters
        vec3 enhancedSSS = calculateEnhancedSSS(L, N, V, radiance);

        // Blend both approaches for maximum realism
        totalLighting += disneySSS * 0.6 + enhancedSSS * 0.4;
    }

    // Combine with global illumination
    vec3 finalColor = globalIllum * 0.3 + totalLighting;

    // Professional tone mapping (ACES)
    finalColor = finalColor * 0.8; // Exposure control
    finalColor = (finalColor * (2.51 * finalColor + 0.03)) / (finalColor * (2.43 * finalColor + 0.59) + 0.14);

    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));

    FragColor = vec4(finalColor, 1.0);
}

)";

class SceneDemo {
private:
    GLFWwindow* window;
    GLuint shaderProgram;
    std::vector<Mesh> meshes;
    glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 15.0f); // BETTER starting position
    float cameraSpeed = 5.0f; // Faster movement

public:
    bool initialize() {
        if (!glfwInit()) return false;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(1200, 800, "Professional Scene with SSS", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        if (glewInit() != GLEW_OK) return false;

        createShaders();

        // ALWAYS generate test spheres first
        std::cout << "Generating test geometry..." << std::endl;
        generateComplexScene();

        // THEN try to load professional models (additive)
        if (loadModel("models/sponza/sponza.obj")) {
            std::cout << "Added Sponza to scene!" << std::endl;
        }
        if (loadModel("models/bistro/bistro.obj")) {
            std::cout << "Added Bistro to scene!" << std::endl;
        }
        if (loadModel("models/bunny.obj")) {
            std::cout << "Added Bunny to scene!" << std::endl;
        }
        if (loadModel("models/lucy.obj")) {
            std::cout << "Added Lucy to scene!" << std::endl;
        }

        std::cout << "Total meshes in scene: " << meshes.size() << std::endl;

        // Try to load a professional model, fallback to procedural if not found
        if (!loadModel("models/sponza/sponza.obj") && 
            !loadModel("models/bistro/bistro.obj") &&
            !loadModel("models/bunny.obj") &&
            !loadModel("models/lucy.obj")) {

            std::cout << "No professional models found. Generating complex test geometry..." << std::endl;
            generateComplexScene();
        }

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

        std::cout << "=== PROFESSIONAL SCENE DEMO ===" << std::endl;
        std::cout << "Features: Real-time subsurface scattering on complex geometry" << std::endl;
        std::cout << "Controls: WASD to move camera" << std::endl;
        return true;
    }

    bool loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, 
            aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | 
            aiProcess_GenNormals | aiProcess_PreTransformVertices);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "❌ Failed to load model: " << path << std::endl;
            std::cout << "   Error: " << importer.GetErrorString() << std::endl;
            return false;
        }

        std::cout << "✅ Loading model: " << path << std::endl;
        std::cout << "   Meshes: " << scene->mNumMeshes << std::endl;
        std::cout << "   Materials: " << scene->mNumMaterials << std::endl;

        size_t startMeshCount = meshes.size();
        processNode(scene->mRootNode, scene);
        size_t endMeshCount = meshes.size();

        std::cout << "   Added " << (endMeshCount - startMeshCount) << " mesh objects" << std::endl;
        return true;
    }


    void processNode(aiNode* node, const aiScene* scene) {
        // Process all meshes in current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        // Process child nodes recursively
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            // Position
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            // Normal
            if (mesh->HasNormals()) {
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            }

            // Texture coordinates
            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        Mesh result;
        result.vertices = vertices;
        result.indices = indices;
        result.setupMesh();
        return result;
    }

    void generateComplexScene() {
        // Generate multiple objects for testing SSS
        generateSphere(glm::vec3(0, 0, 0), 1.0f);
        generateSphere(glm::vec3(-3, 0, 0), 0.8f);
        generateSphere(glm::vec3(3, 0, 0), 1.2f);
        generateSphere(glm::vec3(0, 2.5, 0), 0.6f);
        generatePlane(glm::vec3(0, -2, 0), 10.0f);

        std::cout << "Generated complex test scene with " << meshes.size() << " objects" << std::endl;
    }

    void generateSphere(glm::vec3 center, float radius) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        const int segments = 64;
        for (int y = 0; y <= segments; ++y) {
            for (int x = 0; x <= segments; ++x) {
                float xSegment = (float)x / (float)segments;
                float ySegment = (float)y / (float)segments;
                float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
                float yPos = cos(ySegment * PI);
                float zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);

                Vertex vertex;
                vertex.Position = center + glm::vec3(xPos, yPos, zPos) * radius;
                vertex.Normal = glm::vec3(xPos, yPos, zPos);
                vertex.TexCoords = glm::vec2(xSegment, ySegment);
                vertices.push_back(vertex);
            }
        }

        for (int y = 0; y < segments; ++y) {
            for (int x = 0; x < segments; ++x) {
                int current = y * (segments + 1) + x;
                int next = current + segments + 1;

                indices.insert(indices.end(), {current, next, current + 1});
                indices.insert(indices.end(), {next, next + 1, current + 1});
            }
        }

        Mesh mesh;
        mesh.vertices = vertices;
        mesh.indices = indices;
        mesh.setupMesh();
        meshes.push_back(mesh);
    }

    void generatePlane(glm::vec3 center, float size) {
        std::vector<Vertex> vertices = {
            {{center.x - size, center.y, center.z - size}, {0, 1, 0}, {0, 0}},
            {{center.x + size, center.y, center.z - size}, {0, 1, 0}, {1, 0}},
            {{center.x + size, center.y, center.z + size}, {0, 1, 0}, {1, 1}},
            {{center.x - size, center.y, center.z + size}, {0, 1, 0}, {0, 1}}
        };

        std::vector<unsigned int> indices = {0, 1, 2, 2, 3, 0};

        Mesh mesh;
        mesh.vertices = vertices;
        mesh.indices = indices;
        mesh.setupMesh();
        meshes.push_back(mesh);
    }

    void createShaders() {
        GLuint vertexShader = compileShader(sceneVertexShader, GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader(sceneFragmentShader, GL_FRAGMENT_SHADER);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        GLint success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cerr << "Shader linking failed: " << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    GLuint compileShader(const char* source, GLenum type) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        }
        return shader;
    }

    void run() {
        while (!glfwWindowShouldClose(window)) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            processInput();
            render();

            glfwSwapBuffers(window);
            glfwPollEvents();

            auto frameEnd = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
            if (frameTime > 16.67f) {
                std::cout << "Frame time: " << frameTime << "ms" << std::endl;
            }
        }
    }

    void processInput() {
        float deltaTime = 0.016f; // ~60fps
        float cameraSpeed = 2.5f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * glm::vec3(0, 0, -1);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos += cameraSpeed * glm::vec3(0, 0, 1);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos += cameraSpeed * glm::vec3(-1, 0, 0);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += cameraSpeed * glm::vec3(1, 0, 0);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    void render() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        float time = glfwGetTime();

        // FIXED: Better camera positioning for loaded models
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::mat4 view = glm::lookAt(cameraPos, target, glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1200.0f / 800.0f, 0.1f, 1000.0f); // Increased far plane

        // Enhanced lighting setup
        glm::vec3 lightPositions[] = {
            glm::vec3(sin(time * 0.5f) * 15.0f, 8.0f, cos(time * 0.5f) * 15.0f),  // Bigger orbit
            glm::vec3(-sin(time * 0.3f) * 10.0f, 5.0f, -cos(time * 0.3f) * 10.0f),
            glm::vec3(8.0f, 4.0f, 8.0f),
            glm::vec3(-8.0f, 4.0f, -8.0f)
        };
        glm::vec3 lightColors[] = {
            glm::vec3(4.0f, 3.5f, 3.0f),
            glm::vec3(3.0f, 3.5f, 4.0f),
            glm::vec3(3.5f, 4.0f, 3.5f),
            glm::vec3(3.8f, 3.8f, 3.8f)
        };

        // Mark's SSS parameters
        glUniform3f(glGetUniformLocation(shaderProgram, "scatteringCoeff"), 0.8f, 0.6f, 0.4f);
        glUniform3f(glGetUniformLocation(shaderProgram, "absorptionCoeff"), 0.1f, 0.3f, 0.6f);
        glUniform1f(glGetUniformLocation(shaderProgram, "scatteringDistance"), 0.3f);
        glUniform3f(glGetUniformLocation(shaderProgram, "internalColor"), 0.9f, 0.5f, 0.3f);
        glUniform1f(glGetUniformLocation(shaderProgram, "thickness"), 0.4f);
        glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), 0.5f);
        glUniform1f(glGetUniformLocation(shaderProgram, "subsurfaceMix"), 0.8f);

        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPositions"), 4, glm::value_ptr(lightPositions[0]));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColors"), 4, glm::value_ptr(lightColors[0]));
        glUniform3fv(glGetUniformLocation(shaderProgram, "camPos"), 1, glm::value_ptr(cameraPos));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // FIXED: Render each mesh with appropriate scaling
        for (size_t i = 0; i < meshes.size(); ++i) {
            glm::mat4 model = glm::mat4(1.0f);

            // Different scaling and positioning for different objects
            if (i < 5) {
                // First 5 meshes are our generated spheres - keep them small
                model = glm::scale(model, glm::vec3(1.0f));
            } else {
                // Professional models - scale them appropriately
                if (i == 5) {
                    // First loaded model (likely bunny or lucy) - center and scale
                    model = glm::translate(model, glm::vec3(5.0f, 0.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(0.01f)); // Scale down big models
                } else if (i == 6) {
                    // Second model
                    model = glm::translate(model, glm::vec3(-5.0f, 0.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(0.01f));
                } else {
                    // Other models - spread them out
                    float angle = (i - 7) * 60.0f * PI / 180.0f;
                    model = glm::translate(model, glm::vec3(cos(angle) * 8.0f, 0.0f, sin(angle) * 8.0f));
                    model = glm::scale(model, glm::vec3(0.005f)); // Even smaller for complex models
                }
            }

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            meshes[i].Draw();
        }

        // Debug output
        static int frameCount = 0;
        if (++frameCount % 60 == 0) {
            std::cout << "Rendering " << meshes.size() << " meshes. Camera at (" 
                      << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
        }
    }

};

int main() {
    SceneDemo demo;
    if (!demo.initialize()) {
        std::cerr << "Failed to initialize scene demo" << std::endl;
        return -1;
    }

    std::cout << "\n=== TO GET PROFESSIONAL MODELS ===" << std::endl;
    std::cout << "Download models and place in:" << std::endl;
    std::cout << "  models/sponza/sponza.obj (Intel Sponza)" << std::endl;
    std::cout << "  models/bistro/bistro.obj (Lumberyard Bistro)" << std::endl;
    std::cout << "  models/bunny.obj (Stanford Bunny)" << std::endl;
    std::cout << "  models/lucy.obj (Stanford Lucy)" << std::endl;

    demo.run();
    glfwTerminate();
    return 0;
}

