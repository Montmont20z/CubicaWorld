#include "Texture.hpp"
#include "Camera.hpp"
#include <string>
#include <stb/stb_image.h>
#include <iostream>

Texture::Texture(const std::string& path, GLenum texType, GLenum slot, GLenum pixelType )
    : type_(texType)
{
    int widthImg, heightImg, numColCh;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* bytes = stbi_load(path.c_str(), &widthImg, &heightImg, &numColCh, 0);
    
    if (!bytes){
        throw std::runtime_error("Failed to load texture: " + path);
    }

    glGenTextures(1, &ID);
    glBindTexture(type_, ID);
    
    glTexParameteri(type_, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // last parameter can be GL_NEAREST or GL_LINEAR. GL_NEAREST result in pixel image, GL_LINEAR result in blur image
    glTexParameteri(type_, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // last parameter can be GL_NEAREST or GL_LINEAR. GL_NEAREST result in pixel image, GL_LINEAR result in blur image
    // Axis from x,y,z to s,t,r
    // s,t,r -> GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R
    // Options for last param (GL_REPEAT, GL_MIRROR_REPEAT, GL_CAMP_TO_EDGE, GL_CAMP_TO_BORDER)
    glTexParameteri(type_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(type_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // For GL_CAMP_TO_BORDER
    // float flatColor[] = {1.0f,1.0f,1.0f,1.0f};
    // glTexParameterfv(type, GL_TEXTURE_BORDER_COLOR, flatColor); 
    GLenum fmt = (numColCh == 4) ? GL_RGBA : (numColCh == 1) ? GL_RED : GL_RGB;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(type_, 0, fmt, widthImg, heightImg, 0, fmt, pixelType, bytes);
    glGenerateMipmap(type_);

    glBindTexture(type_, 0);
    stbi_image_free(bytes);     // free up heap resources
}

Texture::~Texture() noexcept
{
    if (ID != 0) {
        glDeleteTextures(1, &ID);
        ID = 0;
    }
}

// move constructor
Texture::Texture(Texture &&other) noexcept
    : ID(other.ID)
    , type_(other.type_)
{
    other.ID = 0;
}

Texture &Texture::operator=(Texture &&other) noexcept
{
    if (this != &other){ // protect against self move
        if (ID != 0){ // delete resource other current texture object, then replace with other object's ID
            glDeleteTextures(1, &ID);
        }
        ID = other.ID;
        type_ = other.type_;
        
        other.ID = 0;
    }
    return *this;
}

void Texture::Bind(GLuint unit) const noexcept
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(type_, ID);
}

void Texture::Unbind() const noexcept
{
    glBindTexture(type_, 0);
}
