#pragma once
#include <glad/glad.h>
#include <string>

std::string get_file_content(const char* filename);

class Shader
{
private:
public:
    GLuint ID = 0;
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
    // void Delete();
    

};



