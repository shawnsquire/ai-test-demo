#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) noexcept;
    Shader& operator=(Shader&&) noexcept;

    void use() const;

    // generic setters
    void set(const std::string& name, int v)              const;
    void set(const std::string& name, float v)            const;
    void set(const std::string& name, const glm::vec3& v) const;
    void set(const std::string& name, const glm::mat4& v) const;
    void set(const std::string& name, const glm::vec3* v, int count) const;


    unsigned int id() const { return program_; }

private:
    unsigned int program_{};
    unsigned int compile(const std::string& src, unsigned int type);
    std::string  loadFile(const std::string& path);
};

