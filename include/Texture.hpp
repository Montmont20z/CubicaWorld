#pragma once
#include <glad/glad.h>
#include <string>
#include "Shader.hpp"

class Texture {
private:
    GLuint type{GL_TEXTURE_2D};
public:
    GLuint ID{0};
    Texture(const std::string& path, GLenum texType, GLenum slot, GLenum format, GLenum pixelType );
    void texUnit(Shader &shader, const std::string &uniform, GLuint unit);
    ~Texture() noexcept;

    // non copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    // movable
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    void Bind(GLuint unit) const noexcept;
    void Unbind() const noexcept;


};