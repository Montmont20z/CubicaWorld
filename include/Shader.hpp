#pragma once
#include <glad/glad.h>
#include <string>
#include <Texture.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
private:
public:
    GLuint ID{0};
    explicit Shader(const std::string& vertexFile, const std::string& fragmentFile);
    ~Shader() noexcept;
    
    // non-copyable
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    /* Prevent this 
    Shader a("a.vert", "b.frag");
    Shader b = a;          // ❌ compile error
    b = a;                 // ❌ compile error
    */
    
    // movable
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other)noexcept;
    /*Allow this
    Shader createShader(){
        Shader s("a.vert", "b.frag");
        return s; // moved, not copies
    }
    */
    
    void Activate() const noexcept;
    void AttachTextureUnit(const GLuint textureID, const std::string &uniform);
    
    void SetVec3(const std::string& name, const glm::vec3& value) {
        Activate();
        glUniform3f(glGetUniformLocation(ID, name.c_str()), 
                    value.x, value.y, value.z);
    }
    
    void SetVec4(const std::string& name, const glm::vec4& value) {
        Activate();
        glUniform4f(glGetUniformLocation(ID, name.c_str()), 
                    value.x, value.y, value.z, value.w);
    }
    
    void SetMat4(const std::string& name, const glm::mat4& value) {
        Activate();
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 
                          1, GL_FALSE, glm::value_ptr(value));
    }
};



