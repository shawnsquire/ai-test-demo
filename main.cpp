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
#include <map>

const float PI = 3.14159265359f;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct ModelInfo {
    std::string name;
    std::vector<size_t> meshIndices;
    glm::vec3 idealScale;
    glm::vec3 idealPosition;
    glm::vec3 cameraDistance;
    std::string description;
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

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
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

const char* sexyVertexShader = R"(
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
out vec3 ViewPos;

void main() {
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    ViewPos = vec3(view * vec4(WorldPos, 1.0));

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
)";

const char* sexyFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 ViewPos;

uniform vec3 scatteringCoeff;
uniform vec3 absorptionCoeff;
uniform float scatteringDistance;
uniform vec3 internalColor;
uniform float thickness;
uniform float roughness;
uniform float subsurfaceMix;
uniform float materialType;

uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform vec3 camPos;
uniform float time;

const float PI = 3.14159265359;

vec3 calculateDisneySSS(vec3 L, vec3 N, vec3 V, vec3 lightColor, vec3 albedo) {
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    vec3 H = normalize(L + V);
    float HdotL = max(dot(H, L), 0.0);

    if (NdotL <= 0.0 || NdotV <= 0.0) return vec3(0.0);

    float alpha = roughness * roughness;
    float FD90 = 0.5 + 2.0 * sqrt(alpha) * HdotL * HdotL;
    float fd = (1.0 + (FD90 - 1.0) * pow(1.0 - NdotL, 5.0)) * 
               (1.0 + (FD90 - 1.0) * pow(1.0 - NdotV, 5.0));

    float FSS90 = sqrt(alpha) * HdotL * HdotL;
    float FSS = (1.0 + (FSS90 - 1.0) * pow(1.0 - NdotL, 5.0)) * 
                (1.0 + (FSS90 - 1.0) * pow(1.0 - NdotV, 5.0));

    float fss = (1.0 / (NdotL * NdotV) - 0.5) * FSS + 0.5;

    vec3 rho_ss = albedo;
    float kss = subsurfaceMix;

    vec3 fdiff = NdotL * NdotV * (rho_ss / PI) * ((1.0 - kss) * fd + 1.25 * kss * fss);
    return fdiff * lightColor;
}

vec3 calculateEnhancedSSS(vec3 L, vec3 N, vec3 V, vec3 lightColor) {
    float wrap = scatteringDistance;
    float NdotL = dot(N, L);
    float wrappedDiffuse = max(0.0, (NdotL + wrap) / (1.0 + wrap));

    float materialMultiplier = 1.0;
    vec3 materialTint = vec3(1.0);

    if (materialType < 0.5) {
        materialMultiplier = 1.0;
        materialTint = vec3(1.0, 0.8, 0.6);
    } else if (materialType < 1.5) {
        materialMultiplier = 0.7;
        materialTint = vec3(0.95, 0.95, 1.0);
    } else if (materialType < 2.5) {
        materialMultiplier = 1.2;
        materialTint = vec3(1.0, 0.9, 0.7);
    } else {
        materialMultiplier = 0.8;
        materialTint = vec3(0.7, 1.0, 0.8);
    }

    vec3 H = normalize(L + N * scatteringDistance);
    float VdotH = max(0.0, dot(-V, H));
    float transmission = pow(VdotH, 3.0) * thickness * materialMultiplier;

    vec3 scatteredLight = lightColor * scatteringCoeff * wrappedDiffuse * materialTint;
    vec3 transmittedLight = lightColor * internalColor * transmission * materialTint;

    vec3 attenuation = exp(-absorptionCoeff * scatteringDistance * 2.0);
    return (scatteredLight + transmittedLight) * attenuation;
}

vec3 calculateRimLighting(vec3 N, vec3 V, vec3 lightColor) {
    float rimPower = 2.0;
    float rimIntensity = 0.5;
    float rim = 1.0 - max(0.0, dot(N, V));
    rim = pow(rim, rimPower) * rimIntensity;
    return lightColor * rim * vec3(0.8, 0.9, 1.0);
}

vec3 calculateSceneGI(vec3 worldPos, vec3 normal) {
    vec3 ambient = vec3(0.08, 0.08, 0.12);

    float skyFactor = max(0.0, dot(normal, vec3(0, 1, 0)));
    vec3 skyContribution = vec3(0.4, 0.6, 1.0) * skyFactor * 0.2;

    float groundFactor = max(0.0, dot(normal, vec3(0, -1, 0)));
    vec3 groundContribution = vec3(0.8, 0.6, 0.4) * groundFactor * 0.05;

    return ambient + skyContribution + groundContribution;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    vec3 globalIllum = calculateSceneGI(WorldPos, N);
    vec3 albedo = vec3(0.8, 0.6, 0.5);

    vec3 totalLighting = vec3(0.0);
    vec3 totalRim = vec3(0.0);

    for(int i = 0; i < 4; ++i) {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance + 1.0);
        vec3 radiance = lightColors[i] * attenuation;

        vec3 disneySSS = calculateDisneySSS(L, N, V, radiance, albedo);
        vec3 enhancedSSS = calculateEnhancedSSS(L, N, V, radiance);

        totalLighting += disneySSS * 0.06 + enhancedSSS * 0.94;
        totalRim += calculateRimLighting(N, V, radiance);
    }

    vec3 finalColor = globalIllum * 0.2 + totalLighting + totalRim * 0.3;

    finalColor = finalColor * 1.2;
    finalColor = (finalColor * (2.51 * finalColor + 0.03)) / (finalColor * (2.43 * finalColor + 0.59) + 0.14);

    finalColor *= vec3(1.05, 1.0, 0.95);

    finalColor = pow(finalColor, vec3(1.0/2.2));
    FragColor = vec4(finalColor, 1.0);
}
)";

class SexySSDemo {
private:
    GLFWwindow* window;
    GLuint shaderProgram;
    std::vector<Mesh> meshes;
    std::vector<ModelInfo> models;
    int currentModel = 0;
    bool showAllModels = false;

    glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 8.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    float cameraDistance = 8.0f;
    float cameraAngle = 0.0f;
    bool autoRotate = true;

    int currentMaterial = 0;
    const char* materialNames[4] = {"Skin", "Marble", "Wax", "Jade"};

    GLuint compileShader(const char* source, GLenum shaderType) {
        GLuint shader = glCreateShader(shaderType);
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

    void generateSphere(glm::vec3 center, float radius) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        const int latSegments = 20;
        const int lonSegments = 40;

        for (int lat = 0; lat <= latSegments; ++lat) {
            float theta = lat * PI / latSegments;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            for (int lon = 0; lon <= lonSegments; ++lon) {
                float phi = lon * 2 * PI / lonSegments;
                float sinPhi = sin(phi);
                float cosPhi = cos(phi);

                Vertex vertex;
                vertex.Position = center + radius * glm::vec3(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
                vertex.Normal = glm::normalize(vertex.Position - center);
                vertex.TexCoords = glm::vec2(float(lon) / lonSegments, float(lat) / latSegments);

                vertices.push_back(vertex);
            }
        }

        for (int lat = 0; lat < latSegments; ++lat) {
            for (int lon = 0; lon < lonSegments; ++lon) {
                int current = lat * (lonSegments + 1) + lon;
                int next = current + lonSegments + 1;

                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);

                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }

        Mesh mesh;
        mesh.vertices = vertices;
        mesh.indices = indices;
        mesh.setupMesh();
        meshes.push_back(mesh);
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

        size_t startMeshCount = meshes.size();
        processNode(scene->mRootNode, scene);
        size_t endMeshCount = meshes.size();

        std::cout << "   Added " << (endMeshCount - startMeshCount) << " mesh objects" << std::endl;
        return true;
    }

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    void processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            vertex.Position.x = mesh->mVertices[i].x;
            vertex.Position.y = mesh->mVertices[i].y;
            vertex.Position.z = mesh->mVertices[i].z;

            if (mesh->HasNormals()) {
                vertex.Normal.x = mesh->mNormals[i].x;
                vertex.Normal.y = mesh->mNormals[i].y;
                vertex.Normal.z = mesh->mNormals[i].z;
            } else {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        Mesh meshObj;
        meshObj.vertices = vertices;
        meshObj.indices = indices;
        meshObj.setupMesh();
        meshes.push_back(meshObj);
    }

public:
    bool initialize() {
        if (!glfwInit()) return false;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        window = glfwCreateWindow(1400, 900, "🔥 Sexy Subsurface Scattering Demo 🔥", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, keyCallback);

        if (glewInit() != GLEW_OK) return false;

        createShaders();
        loadAllModels();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);

        printControls();
        return true;
    }

    void createShaders() {
        GLuint vertexShader = compileShader(sexyVertexShader, GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader(sexyFragmentShader, GL_FRAGMENT_SHADER);

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

    void loadAllModels() {
        generateTestSpheres();

        loadModelWithInfo("models/bunny.obj", "Stanford Bunny", "Classic test model with complex geometry", 
                         glm::vec3(0.1f), glm::vec3(0, 0, 0), glm::vec3(0, 0, 6));

        loadModelWithInfo("models/lucy.obj", "Stanford Lucy", "High-detail scan perfect for SSS", 
                         glm::vec3(0.005f), glm::vec3(3, 0, 0), glm::vec3(0, 0, 10));

        loadModelWithInfo("models/dragon.obj", "Stanford Dragon", "Complex surface details showcase", 
                         glm::vec3(0.008f), glm::vec3(-3, 0, 0), glm::vec3(0, 0, 8));

        loadModelWithInfo("models/sponza/sponza.obj", "Intel Sponza", "Architectural test scene", 
                         glm::vec3(0.01f), glm::vec3(0, -2, 0), glm::vec3(0, 5, 15));

        std::cout << "🎨 Loaded " << models.size() << " model groups with " << meshes.size() << " total meshes" << std::endl;
    }

    void generateTestSpheres() {
        ModelInfo sphereModel;
        sphereModel.name = "Test Spheres";
        sphereModel.description = "Procedural spheres for SSS testing";
        sphereModel.idealScale = glm::vec3(1.0f);
        sphereModel.idealPosition = glm::vec3(0, 0, 0);
        sphereModel.cameraDistance = glm::vec3(0, 2, 8);

        size_t startIdx = meshes.size();

        generateSphere(glm::vec3(0, 0, 0), 1.0f);
        generateSphere(glm::vec3(-2.5f, 0, 0), 0.8f);
        generateSphere(glm::vec3(2.5f, 0, 0), 1.2f);
        generateSphere(glm::vec3(0, 2.0f, 0), 0.6f);

        for (size_t i = startIdx; i < meshes.size(); ++i) {
            sphereModel.meshIndices.push_back(i);
        }

        models.push_back(sphereModel);
    }

    bool loadModelWithInfo(const std::string& path, const std::string& name, 
                          const std::string& desc, glm::vec3 scale, glm::vec3 pos, glm::vec3 camDist) {
        size_t startIdx = meshes.size();

        if (!loadModel(path)) {
            return false;
        }

        ModelInfo modelInfo;
        modelInfo.name = name;
        modelInfo.description = desc;
        modelInfo.idealScale = scale;
        modelInfo.idealPosition = pos;
        modelInfo.cameraDistance = camDist;

        for (size_t i = startIdx; i < meshes.size(); ++i) {
            modelInfo.meshIndices.push_back(i);
        }

        models.push_back(modelInfo);
        return true;
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        SexySSDemo* demo = static_cast<SexySSDemo*>(glfwGetWindowUserPointer(window));
        if (action == GLFW_PRESS) {
            demo->handleKeyPress(key);
        }
    }

    void handleKeyPress(int key) {
        switch (key) {
            case GLFW_KEY_SPACE:
                currentModel = (currentModel + 1) % models.size();
                updateCameraForCurrentModel();
                std::cout << "🎯 Now showing: " << models[currentModel].name 
                         << " - " << models[currentModel].description << std::endl;
                break;

            case GLFW_KEY_TAB:
                showAllModels = !showAllModels;
                std::cout << (showAllModels ? "🌟 Showing all models" : "🎯 Single model mode") << std::endl;
                break;

            case GLFW_KEY_M:
                currentMaterial = (currentMaterial + 1) % 4;
                std::cout << "🎨 Material: " << materialNames[currentMaterial] << std::endl;
                break;

            case GLFW_KEY_R:
                autoRotate = !autoRotate;
                std::cout << (autoRotate ? "🔄 Auto-rotation ON" : "⏸️ Auto-rotation OFF") << std::endl;
                break;

            case GLFW_KEY_H:
                printControls();
                break;

            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, true);
                break;
        }
    }

    void updateCameraForCurrentModel() {
        if (currentModel < models.size()) {
            cameraTarget = models[currentModel].idealPosition;
            cameraDistance = glm::length(models[currentModel].cameraDistance);
        }
    }

    void printControls() {
        std::cout << "\n🎮 === SEXY SSS DEMO CONTROLS ===" << std::endl;
        std::cout << "SPACE    - Cycle through models" << std::endl;
        std::cout << "TAB      - Toggle single/all models" << std::endl;
        std::cout << "M        - Cycle material types (Skin/Marble/Wax/Jade)" << std::endl;
        std::cout << "R        - Toggle auto-rotation" << std::endl;
        std::cout << "WASD     - Manual camera control" << std::endl;
        std::cout << "H        - Show this help" << std::endl;
        std::cout << "ESC      - Exit" << std::endl;
        std::cout << "================================\n" << std::endl;
    }

    void processInput() {
        float deltaTime = 0.016f;
        float speed = 3.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraDistance -= speed * 2.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraDistance += speed * 2.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraAngle -= speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraAngle += speed;

        cameraDistance = glm::clamp(cameraDistance, 2.0f, 50.0f);
    }

    void setMaterialUniforms() {
        switch (currentMaterial) {
            case 0: // Skin
                glUniform3f(glGetUniformLocation(shaderProgram, "scatteringCoeff"), 0.9f, 0.7f, 0.5f);
                glUniform3f(glGetUniformLocation(shaderProgram, "absorptionCoeff"), 0.1f, 0.3f, 0.6f);
                glUniform1f(glGetUniformLocation(shaderProgram, "scatteringDistance"), 0.4f);
                glUniform3f(glGetUniformLocation(shaderProgram, "internalColor"), 1.0f, 0.6f, 0.4f);
                glUniform1f(glGetUniformLocation(shaderProgram, "thickness"), 0.5f);
                glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), 0.4f);
                glUniform1f(glGetUniformLocation(shaderProgram, "subsurfaceMix"), 0.9f);
                break;
            case 1: // Marble
                glUniform3f(glGetUniformLocation(shaderProgram, "scatteringCoeff"), 0.8f, 0.8f, 0.9f);
                glUniform3f(glGetUniformLocation(shaderProgram, "absorptionCoeff"), 0.05f, 0.05f, 0.1f);
                glUniform1f(glGetUniformLocation(shaderProgram, "scatteringDistance"), 0.6f);
                glUniform3f(glGetUniformLocation(shaderProgram, "internalColor"), 0.9f, 0.9f, 1.0f);
                glUniform1f(glGetUniformLocation(shaderProgram, "thickness"), 0.3f);
                glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), 0.2f);
                glUniform1f(glGetUniformLocation(shaderProgram, "subsurfaceMix"), 0.7f);
                break;
            case 2: // Wax
                glUniform3f(glGetUniformLocation(shaderProgram, "scatteringCoeff"), 1.0f, 0.9f, 0.7f);
                glUniform3f(glGetUniformLocation(shaderProgram, "absorptionCoeff"), 0.2f, 0.4f, 0.8f);
                glUniform1f(glGetUniformLocation(shaderProgram, "scatteringDistance"), 0.8f);
                glUniform3f(glGetUniformLocation(shaderProgram, "internalColor"), 1.0f, 0.8f, 0.6f);
                glUniform1f(glGetUniformLocation(shaderProgram, "thickness"), 0.7f);
                glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), 0.6f);
                glUniform1f(glGetUniformLocation(shaderProgram, "subsurfaceMix"), 0.95f);
                break;
            case 3: // Jade
                glUniform3f(glGetUniformLocation(shaderProgram, "scatteringCoeff"), 0.6f, 0.9f, 0.7f);
                glUniform3f(glGetUniformLocation(shaderProgram, "absorptionCoeff"), 0.3f, 0.1f, 0.2f);
                glUniform1f(glGetUniformLocation(shaderProgram, "scatteringDistance"), 0.3f);
                glUniform3f(glGetUniformLocation(shaderProgram, "internalColor"), 0.7f, 1.0f, 0.8f);
                glUniform1f(glGetUniformLocation(shaderProgram, "thickness"), 0.4f);
                glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), 0.3f);
                glUniform1f(glGetUniformLocation(shaderProgram, "subsurfaceMix"), 0.8f);
                break;
        }
    }

    void render() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        float time = glfwGetTime();

        if (autoRotate) {
            cameraAngle += 0.3f * 0.016f;
        }

        cameraPos = cameraTarget + glm::vec3(
            sin(cameraAngle) * cameraDistance,
            2.0f,
            cos(cameraAngle) * cameraDistance
        );

        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1400.0f / 900.0f, 0.1f, 100.0f);

        glm::vec3 lightPositions[] = {
            glm::vec3(sin(time * 0.3f) * 12.0f, 6.0f, cos(time * 0.3f) * 12.0f),
            glm::vec3(-sin(time * 0.5f) * 8.0f, 4.0f, -cos(time * 0.5f) * 8.0f),
            glm::vec3(6.0f, 3.0f, 6.0f),
            glm::vec3(-6.0f, 3.0f, -6.0f)
        };
        glm::vec3 lightColors[] = {
            glm::vec3(5.0f, 4.0f, 3.5f),
            glm::vec3(3.5f, 4.0f, 5.0f),
            glm::vec3(4.0f, 5.0f, 4.0f),
            glm::vec3(4.5f, 4.5f, 4.5f)
        };

        setMaterialUniforms();

        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPositions"), 4, glm::value_ptr(lightPositions[0]));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColors"), 4, glm::value_ptr(lightColors[0]));
        glUniform3fv(glGetUniformLocation(shaderProgram, "camPos"), 1, glm::value_ptr(cameraPos));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(shaderProgram, "time"), time);
        glUniform1f(glGetUniformLocation(shaderProgram, "materialType"), float(currentMaterial));

        if (showAllModels) {
            renderAllModels();
        } else {
            renderSingleModel(currentModel);
        }
    }

    void renderSingleModel(int modelIndex) {
        if (modelIndex >= models.size()) return;

        const ModelInfo& model = models[modelIndex];
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, model.idealPosition);
        modelMatrix = glm::scale(modelMatrix, model.idealScale);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

        for (size_t meshIdx : model.meshIndices) {
            if (meshIdx < meshes.size()) {
                meshes[meshIdx].Draw();
            }
        }
    }

    void renderAllModels() {
        for (size_t i = 0; i < models.size(); ++i) {
            const ModelInfo& model = models[i];
            glm::mat4 modelMatrix = glm::mat4(1.0f);

            float angle = (float(i) / float(models.size())) * 2.0f * PI;
            glm::vec3 offset = glm::vec3(cos(angle) * 8.0f, 0, sin(angle) * 8.0f);

            modelMatrix = glm::translate(modelMatrix, model.idealPosition + offset);
            modelMatrix = glm::scale(modelMatrix, model.idealScale * 0.7f);

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

            for (size_t meshIdx : model.meshIndices) {
                if (meshIdx < meshes.size()) {
                    meshes[meshIdx].Draw();
                }
            }
        }
    }

    void run() {
        while (!glfwWindowShouldClose(window)) {
            processInput();
            render();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
};

int main() {
    SexySSDemo demo;
    if (!demo.initialize()) {
        std::cerr << "❌ Failed to initialize sexy SSS demo" << std::endl;
        return -1;
    }

    demo.run();
    glfwTerminate();
    return 0;
}

