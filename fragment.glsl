#version 330 core

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;

out vec4 FragColor;

// Subsurface scattering parameters
const float scatteringStrength = 0.8;
const float translucency = 0.4;
const vec3 skinColor = vec3(0.9, 0.7, 0.6);
const vec3 scatterColor = vec3(1.0, 0.4, 0.3);

vec3 calculateSubsurfaceScattering(vec3 lightDir, vec3 viewDir, vec3 normal) {
    // Direct lighting
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = skinColor * NdotL;

    // Subsurface scattering approximation
    // Light penetrates surface and scatters
    float backlight = max(0.0, dot(-lightDir, viewDir));
    float subsurface = pow(backlight, 4.0) * scatteringStrength;

    // Translucency effect - light passing through thin areas
    float thickness = 1.0 - max(0.0, dot(normal, lightDir));
    float translucent = pow(thickness, 2.0) * translucency;

    // Combine effects
    vec3 scattering = scatterColor * (subsurface + translucent);

    return diffuse + scattering;
}

vec3 calculateGlobalIllumination() {
    // Simple ambient/indirect lighting approximation
    vec3 ambient = vec3(0.1, 0.1, 0.15) * skinColor;

    // Fake bounce light from "environment"
    vec3 upVector = vec3(0, 1, 0);
    float skyContribution = max(0.0, dot(Normal, upVector)) * 0.3;
    vec3 skyLight = vec3(0.4, 0.6, 1.0) * skyContribution;

    return ambient + skyLight;
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Calculate subsurface scattering
    vec3 subsurfaceColor = calculateSubsurfaceScattering(lightDir, viewDir, norm);

    // Add global illumination
    vec3 globalIllum = calculateGlobalIllumination();

    // Specular highlight
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = vec3(0.3) * spec;

    // Final color combining all effects
    vec3 finalColor = subsurfaceColor + globalIllum + specular;

    FragColor = vec4(finalColor, 1.0);
}

