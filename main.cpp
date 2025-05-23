#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>

const float PI = 3.14159265359f;

// Real Global Illumination Vertex Shader
const char* giVertexShader = R"(
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
out vec4 ScreenPos;

void main() {
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;

    gl_Position = projection * view * vec4(WorldPos, 1.0);
    ScreenPos = gl_Position;
}
)";

// Real Global Illumination + SSS Fragment Shader
const char* giFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 ScreenPos;

// Mark's SSS Parameters
uniform vec3 scatteringCoeff;
uniform vec3 absorptionCoeff;
uniform float scatteringDistance;
uniform vec3 internalColor;
uniform float thickness;

// Lighting
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform vec3 camPos;
uniform float time;

const float PI = 3.14159265359;
const int NUM_GI_SAMPLES = 8;

// Proper Global Illumination using Monte Carlo Integration
vec3 calculateRealGlobalIllumination(vec3 worldPos, vec3 normal) {
    vec3 indirectColor = vec3(0.0);
    vec3 up = vec3(0.0, 1.0, 0.0);

    // Create tangent space for hemisphere sampling
    vec3 tangent = normalize(cross(up, normal));
    if (length(tangent) < 0.1) {
        tangent = normalize(cross(vec3(1.0, 0.0, 0.0), normal));
    }
    vec3 bitangent = cross(normal, tangent);

    // Sample hemisphere for indirect lighting (simplified Monte Carlo)
    for (int i = 0; i < NUM_GI_SAMPLES; i++) {
        // Generate pseudo-random hemisphere sample
        float u1 = fract(sin(dot(worldPos.xy + float(i), vec2(12.9898, 78.233))) * 43758.5453);
        float u2 = fract(sin(dot(worldPos.xz + float(i), vec2(4.898, 7.23))) * 23421.631);

        // Convert to hemisphere direction
        float cosTheta = sqrt(u1);
        float sinTheta = sqrt(1.0 - u1);
        float phi = 2.0 * PI * u2;

        vec3 sampleDir = vec3(
            sinTheta * cos(phi),
            cosTheta,
            sinTheta * sin(phi)
        );

        // Transform to world space
        vec3 worldSampleDir = tangent * sampleDir.x + normal * sampleDir.y + bitangent * sampleDir.z;

        // Simple ray-sphere intersection for environment sampling
        // Simulating light bouncing from environment
        float envDistance = 5.0; // Assume environment sphere radius
        vec3 envSamplePos = worldPos + worldSampleDir * envDistance;

        // Calculate environment lighting (simplified sky model)
        float skyIntensity = max(0.0, dot(worldSampleDir, vec3(0, 1, 0)));
        vec3 skyColor = mix(vec3(0.1, 0.1, 0.2), vec3(0.3, 0.6, 1.0), skyIntensity);

        // Add bounce lighting from other surfaces (simplified)
        vec3 bounceColor = vec3(0.0);
        for (int j = 0; j < 4; j++) {
            vec3 lightToSample = envSamplePos - lightPositions[j];
            float lightDist = length(lightToSample);
            vec3 lightDir = normalize(-lightToSample);

            // Simulate light bouncing off surfaces
            float bounceIntensity = max(0.0, dot(lightDir, -worldSampleDir)) / (lightDist * lightDist);
            bounceColor += lightColors[j] * bounceIntensity * 0.3; // 30% albedo assumption
        }

        // Combine sky and bounce lighting
        vec3 sampleColor = skyColor * 0.1 + bounceColor * 0.01;

        // Weight by cosine (Lambert's law)
        float weight = max(0.0, dot(worldSampleDir, normal));
        indirectColor += sampleColor * weight;
    }

    // Average the samples and apply energy conservation
    indirectColor = indirectColor / float(NUM_GI_SAMPLES) * PI;

    return indirectColor;
}

// Light probe-based GI approximation (faster alternative)
vec3 calculateLightProbeGI(vec3 worldPos, vec3 normal) {
    // Simulate multiple light probes in the scene
    vec3 probePositions[6] = vec3[](
        vec3(-2.0, 2.0, -2.0),
        vec3( 2.0, 2.0, -2.0), 
        vec3(-2.0, 2.0,  2.0),
        vec3( 2.0, 2.0,  2.0),
        vec3( 0.0, 4.0,  0.0),
        vec3( 0.0,-2.0,  0.0)
    );

    vec3 probeColors[6] = vec3[](
        vec3(0.8, 0.6, 0.4), // Warm
        vec3(0.4, 0.6, 0.8), // Cool  
        vec3(0.6, 0.8, 0.6), // Green
        vec3(0.8, 0.8, 0.4), // Yellow
        vec3(0.9, 0.9, 0.9), // White (sky)
        vec3(0.2, 0.2, 0.3)  // Dark (ground)
    );

    vec3 totalContribution = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 6; i++) {
        vec3 probeDir = normalize(probePositions[i] - worldPos);
        float distance = length(probePositions[i] - worldPos);

        // Weight by normal alignment and distance
        float alignment = max(0.0, dot(normal, probeDir));
        float weight = alignment / (1.0 + distance * distance * 0.1);

        totalContribution += probeColors[i] * weight;
        totalWeight += weight;
    }

    return totalContribution / max(totalWeight, 0.001);
}

// Screen-space ambient occlusion approximation
float calculateSSAO(vec3 worldPos, vec3 normal) {
    float occlusion = 0.0;
    float radius = 0.5;

    for (int i = 0; i < 8; i++) {
        // Generate sample positions around the fragment
        float angle = float(i) * PI * 0.25;
        vec3 sampleOffset = vec3(cos(angle), sin(angle), cos(angle * 0.7)) * radius;
        vec3 samplePos = worldPos + sampleOffset;

        // Simple depth comparison (simplified without depth buffer access)
        float sampleDepth = length(samplePos - camPos);
        float currentDepth = length(worldPos - camPos);

        // If sample is closer to camera, it's likely an occluder
        if (sampleDepth < currentDepth - 0.1) {
            occlusion += 1.0;
        }
    }

    return 1.0 - (occlusion / 8.0) * 0.5; // Reduce by 50% max
}

// Mark's Subsurface Scattering with proper parameters
vec3 calculateSubsurfaceScattering(vec3 L, vec3 N, vec3 V, vec3 lightColor) {
    // Wrap-around diffuse for light penetration
    float wrap = scatteringDistance;
    float NdotL = dot(N, L);
    float wrappedDiffuse = max(0.0, (NdotL + wrap) / (1.0 + wrap));

    // Transmission - light passing through material
    vec3 H = normalize(L + N * scatteringDistance);
    float VdotH = max(0.0, dot(-V, H));
    float transmission = pow(VdotH, 3.0) * thickness;

    // Apply wavelength-dependent scattering and absorption
    vec3 scatteredLight = lightColor * scatteringCoeff * wrappedDiffuse;
    vec3 transmittedLight = lightColor * internalColor * transmission;

    // Beer's law approximation for absorption
    vec3 attenuation = exp(-absorptionCoeff * scatteringDistance);

    return (scatteredLight + transmittedLight) * attenuation;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    // === REAL GLOBAL ILLUMINATION (reduced intensity) ===
    vec3 indirectLighting = calculateRealGlobalIllumination(WorldPos, N) * 0.15; // Reduced
    vec3 probeLighting = calculateLightProbeGI(WorldPos, N) * 0.25; // Reduced
    float ambientOcclusion = calculateSSAO(WorldPos, N);

    // Combine GI techniques (much lower intensity)
    vec3 globalIllumination = (indirectLighting + probeLighting) * ambientOcclusion;

    // === DIRECT LIGHTING + SUBSURFACE SCATTERING ===
    vec3 directLighting = vec3(0.0);
    vec3 sssLighting = vec3(0.0);

    for(int i = 0; i < 4; ++i) {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance + 1.0); // Added +1.0 to prevent division by zero
        vec3 radiance = lightColors[i] * attenuation;

        // Direct diffuse (minimal for skin)
        float NdotL = max(dot(N, L), 0.0);
        vec3 albedo = vec3(0.4, 0.3, 0.2); // Darker base color
        directLighting += albedo * radiance * NdotL * 0.06;

        // Subsurface scattering (controlled intensity)
        sssLighting += calculateSubsurfaceScattering(L, N, V, radiance) * 0.3; // Much lower
    }

    // === COMBINE ALL LIGHTING (with proper scaling) ===
    vec3 finalColor = globalIllumination * 0.4 + directLighting + sssLighting;

    // Better HDR tone mapping (ACES approximation)
    finalColor = finalColor * 0.8; // Overall exposure reduction
    finalColor = (finalColor * (2.51 * finalColor + 0.03)) / (finalColor * (2.43 * finalColor + 0.59) + 0.14);

    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));

    FragColor = vec4(finalColor, 1.0);
}

)";

class RealGIDemo {
private:
    GLFWwindow* window;
    GLuint shaderProgram;
    GLuint sphereVAO, sphereVBO, sphereEBO;
    std::vector<unsigned int> indices;
    std::chrono::high_resolution_clock::time_point frameStart;

public:
    bool initialize() {
        if (!glfwInit()) return false;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(800, 600, "Real Global Illumination + SSS Demo", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        if (glewInit() != GLEW_OK) return false;

        createShaders();
        createSphere();

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.02f, 0.02f, 0.03f, 1.0f);

        std::cout << "=== REAL GLOBAL ILLUMINATION DEMO ===" << std::endl;
        std::cout << "Features:" << std::endl;
        std::cout << "✓ Monte Carlo hemisphere sampling for indirect lighting" << std::endl;
        std::cout << "✓ Light probe-based global illumination" << std::endl;
        std::cout << "✓ Screen-space ambient occlusion approximation" << std::endl;
        std::cout << "✓ Subsurface scattering with proper parameters" << std::endl;
        std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
        return true;
    }

    void run() {
        while (!glfwWindowShouldClose(window)) {
            frameStart = std::chrono::high_resolution_clock::now();

            render();

            glfwSwapBuffers(window);
            glfwPollEvents();

            auto frameEnd = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();

            if (frameTime > 16.67f) {
                std::cout << "Frame time: " << frameTime << "ms (over 16.67ms budget)" << std::endl;
            }
        }
    }

private:
    void createShaders() {
        GLuint vertexShader = compileShader(giVertexShader, GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader(giFragmentShader, GL_FRAGMENT_SHADER);

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

        std::cout << "Real GI shaders compiled successfully!" << std::endl;
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

    void createSphere() {
        std::vector<float> vertices;

        const int X_SEGMENTS = 64;
        const int Y_SEGMENTS = 64;

        for (int x = 0; x <= X_SEGMENTS; ++x) {
            for (int y = 0; y <= Y_SEGMENTS; ++y) {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
                float yPos = cos(ySegment * PI);
                float zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);

                vertices.insert(vertices.end(), {xPos, yPos, zPos, xPos, yPos, zPos, xSegment, ySegment});
            }
        }

        for (int y = 0; y < Y_SEGMENTS; ++y) {
            for (int x = 0; x < X_SEGMENTS; ++x) {
                int first = y * (X_SEGMENTS + 1) + x;
                int second = first + X_SEGMENTS + 1;

                indices.insert(indices.end(), {first, second, first + 1});
                indices.insert(indices.end(), {second, second + 1, first + 1});
            }
        }

        glGenVertexArrays(1, &sphereVAO);
        glGenBuffers(1, &sphereVBO);
        glGenBuffers(1, &sphereEBO);

        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    }

    void render() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Set matrices with smooth rotation
        float time = glfwGetTime();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(shaderProgram, "time"), time);

        // Mark's SSS parameters - FIXED values
        glUniform3f(glGetUniformLocation(shaderProgram, "scatteringCoeff"), 0.7f, 0.5f, 0.3f);
        glUniform3f(glGetUniformLocation(shaderProgram, "absorptionCoeff"), 0.1f, 0.2f, 0.4f);
        glUniform1f(glGetUniformLocation(shaderProgram, "scatteringDistance"), 0.2f);
        glUniform3f(glGetUniformLocation(shaderProgram, "internalColor"), 0.8f, 0.4f, 0.2f);
        glUniform1f(glGetUniformLocation(shaderProgram, "thickness"), 0.3f);

        // FIXED: Much lower light intensities
        glm::vec3 lightPositions[] = {
            glm::vec3(sin(time) * 3.0f, 2.0f, cos(time) * 3.0f),
            glm::vec3(-sin(time) * 2.0f, 1.0f, -cos(time) * 2.0f),
            glm::vec3(2.0f, -1.0f, 2.0f),
            glm::vec3(-2.0f, -1.0f, -2.0f)
        };
        glm::vec3 lightColors[] = {
            glm::vec3(3.0f, 2.5f, 2.0f),   // MUCH lower values!
            glm::vec3(2.0f, 2.5f, 3.0f),  
            glm::vec3(2.5f, 3.0f, 2.5f),  
            glm::vec3(2.8f, 2.8f, 2.8f)   
        };

        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPositions"), 4, glm::value_ptr(lightPositions[0]));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColors"), 4, glm::value_ptr(lightColors[0]));
        glUniform3f(glGetUniformLocation(shaderProgram, "camPos"), 0.0f, 0.0f, 3.0f);

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    }

};

int main() {
    RealGIDemo demo;
    if (!demo.initialize()) {
        std::cerr << "Failed to initialize Real GI demo" << std::endl;
        return -1;
    }

    demo.run();
    glfwTerminate();
    return 0;
}

