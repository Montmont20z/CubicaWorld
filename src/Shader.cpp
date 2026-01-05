#include "Shader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

std::string get_file_content(const char *filename)
{
    std::ifstream in(filename, std::ios::binary); // open the file for input, read byte as-is (no conversion) (no new line conversion (\r\n -> \n))
    if(!in){ // check if file is successfully opened
        std::cerr << "Failed to open file: " << filename << std::endl;
        std::abort();
    }
    std::string contents;
    in.seekg(0, std::ios::end); // move the file cursor to end of the file

    auto size = in.tellg(); // return the current cursor position (get file size in byte).
    if (size <= 0){
        std::cerr << "Shader file '" << filename << "' is empty or unreadable." << std::endl;
        std::abort();
    } 
    contents.resize(static_cast<size_t>(size)); // .resize() allocate exactly enough memory to hold the file's content

    in.seekg(0, std::ios::beg); // move cursor back to start of file
    in.read(&contents[0], contents.size()); // read entire file into the content buffer
    in.close(); // close the file
    return(contents);
        
}

// Compile shader and print errors if any
// auto compile_shader = [](GLuint shader) {
bool compile_shader(GLuint shader){
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, ' ');
        glGetShaderInfoLog(shader, logLen, nullptr, &log[0]);
        std::cerr << "Shader compile error:\n" << log << std::endl;
        return false;
    }
    return true;
};

// Link program and print errors if any
// auto link_program = [](GLuint prog) {
bool link_program(GLuint prog){
    GLint success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, ' ');
        glGetProgramInfoLog(prog, logLen, nullptr, &log[0]);
        std::cerr << "Program link error:\n" << log << std::endl;
        return false;
    }
    return true;
};  

Shader::Shader(const char *vertexFile, const char *fragmentFile)
{
    string vertexCode = get_file_content(vertexFile);
    string fragmentCode = get_file_content(fragmentFile);
    
    const char* vertexSource = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();
    
    // Shader creation, source upload, compilation
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); // Allocates a new shader object of type vertex and returns its handle.
    glShaderSource(vertexShader, 1, &vertexSource, nullptr); // Uploads the GLSL source string(s) into the shader object.
    glCompileShader(vertexShader); // Compiles the shader source into GPU-executable code.
    if (!compile_shader(vertexShader)) { 
        std::cerr << "Failed to complied vetex Shader" << std::endl;
        std::abort();
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    if (!compile_shader(fragmentShader)) { 
        std::cerr << "Failed to complied fragment Shader" << std::endl;
        std::abort();
    }

    // Program creation, attach, link
    ID = glCreateProgram(); // Creates a shader program object that will combine (link) vertex and fragment stages.
    // Attaches compiled shader objects to the program.
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID); // resolves attribute locations, varyings, types and builds a final GPU pipeline object you can use with glUseProgram
    if (!link_program(ID)) {
        std::cerr << "Failed to link Shader" << std::endl;
        std::abort();
    }   
    
    // clean up shader
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    // Do not throw from destructor. If context is invalid, glDeleteProgram may be a no-op or cause error;
    // ensure you destroy Shader objects before destroying the GL context.
    if (ID != 0){
        glDeleteProgram(ID);
        ID = 0;
    }
}

void Shader::Activate()
{
    glUseProgram(ID);
}

/* void Shader::Delete()
{
    glDeleteProgram(ID);
}
 */

