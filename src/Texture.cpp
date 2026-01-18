#include "Texture.hpp"
#include <string>
#include <stb/stb_image.h>
#include <iostream>

Texture::Texture(const std::string& path, GLenum texType, GLenum slot, GLenum format, GLenum pixelType )
    : type(texType)
{
    int widthImg, heightImg, numColCh;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* bytes = stbi_load(path.c_str(), &widthImg, &heightImg, &numColCh, 0);
    
    if (!bytes){
        throw std::runtime_error("Failed to load texture: " + path);
    }

    glGenTextures(1, &ID);
    glBindTexture(texType, ID);
    
    glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // last parameter can be GL_NEAREST or GL_LINEAR. GL_NEAREST result in pixel image, GL_LINEAR result in blur image
    glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR); // last parameter can be GL_NEAREST or GL_LINEAR. GL_NEAREST result in pixel image, GL_LINEAR result in blur image
    // Axis from x,y,z to s,t,r
    // s,t,r -> GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R
    // Options for last param (GL_REPEAT, GL_MIRROR_REPEAT, GL_CAMP_TO_EDGE, GL_CAMP_TO_BORDER)
    glTexParameteri(texType, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(texType, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // For GL_CAMP_TO_BORDER
    // float flatColor[] = {1.0f,1.0f,1.0f,1.0f};
    // glTexParameterfv(texType, GL_TEXTURE_BORDER_COLOR, flatColor); 
    glTexImage2D(texType, 0, format, widthImg, heightImg, 0, format, pixelType, bytes);
    glGenerateMipmap(texType);

    glBindTexture(texType, 0);
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
    , type(other.type)
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
        type = other.type;
        
        other.ID = 0;
    }
    return *this;
}

void Texture::Bind(GLuint unit) const noexcept
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(type, ID);
}

void Texture::Unbind() const noexcept
{
    glBindTexture(type, 0);
}
