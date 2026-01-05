#pragma once
#include <glad/glad.h>
#include <string>

std::string get_file_content(const char* filename);

class Shader
{
private:
public:
    GLuint ID;
    Shader(const char* vertexFile, const char* fragmentFile);
    ~Shader();
    
    void Activate();
    // void Delete();
    

};



