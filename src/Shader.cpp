#include "Shader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

// using namespace std;

static std::string get_file_content(const std::string& filename)
{
    std::ifstream in(filename, std::ios::binary); // open the file for input, read byte as-is (no conversion) (no new line conversion (\r\n -> \n))
    if(!in){ // check if file is successfully opened
        throw std::runtime_error("Shader: Failed to open file: " + filename);
    }
    in.seekg(0, std::ios::end); // move the file cursor to end of the file

    auto size = in.tellg(); // return the current cursor position (get file size in byte).
    if (size <= 0){
        throw std::runtime_error("Shader: Shader file: " + filename + " is empty or unreadable");
    } 
    std::string contents;
    contents.resize(static_cast<size_t>(size)); // .resize() allocate exactly enough memory to hold the file's content

    in.seekg(0, std::ios::beg); // move cursor back to start of file
    in.read(&contents[0], contents.size()); // read entire file into the content buffer
    in.close(); // close the file
    return(contents);
        
}

// Compile shader and print errors if any
static bool check_shader_compile(GLuint shader){
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, ' ');
        glGetShaderInfoLog(shader, logLen, nullptr, &log[0]);
        std::cerr << "Shader: Shader compile error:\n" << log << std::endl;
        return false;
    }
    return true;
};

// Link program and print errors if any
static bool check_program_link(GLuint prog){
    GLint success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, ' ');
        glGetProgramInfoLog(prog, logLen, nullptr, &log[0]);
        std::cerr << "Shader: Program link error:\n" << log << std::endl;
        return false;
    }
    return true;
};  

Shader::Shader(const std::string& vertexFile, const std::string& fragmentFile)
// Shader::Shader(const char *vertexFile, const char *fragmentFile)
{
    std::string vertexCode = get_file_content(vertexFile);
    std::string fragmentCode = get_file_content(fragmentFile);
    
    const char* vertexSource = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();
    
    // Shader creation, source upload, compilation
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); // Allocates a new shader object of type vertex and returns its handle.
    glShaderSource(vertexShader, 1, &vertexSource, nullptr); // Uploads the GLSL source string(s) into the shader object.
    glCompileShader(vertexShader); // Compiles the shader source into GPU-executable code.
    if (!check_shader_compile(vertexShader)) { 
        glDeleteShader(vertexShader);
        throw std::runtime_error("Shader: Failed to complied vertex shader");
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    if (!check_shader_compile(fragmentShader)) { 
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw std::runtime_error("Shader: Failed to complied fragment Shader");
    }

    // Program creation, attach, link
    ID = glCreateProgram(); // Creates a shader program object that will combine (link) vertex and fragment stages.
    // Attaches compiled shader objects to the program.
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID); // resolves attribute locations, varyings, types and builds a final GPU pipeline object you can use with glUseProgram
    if (!check_program_link(ID)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(ID);
        throw std::runtime_error("Shader: Failed to link shader");
    }   
    
    // clean up shader
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader() noexcept
{
    // Do not throw from destructor. If context is invalid, glDeleteProgram may be a no-op or cause error;
    // ensure you destroy Shader objects before destroying the GL context.
    if (ID != 0){
        glDeleteProgram(ID);
        ID = 0;
    }
}

// This constructor is called when a Shader is constructed from a temporary or movable object.
/*Eg
Shader a("a.vert", "b.frag");
Shader b = std::move(a);
*/
Shader::Shader(Shader &&other) noexcept 
    : ID(other.ID)
{
    other.ID = 0;
}

Shader &Shader::operator=(Shader &&other) noexcept
{
    if (this != &other){ // Protects against self-move Eg. a = std::move(a);
        // Release current resource (mirror a destructor)
        if (this->ID != 0){
            glDeleteProgram(ID);
        }
        // Move
        this->ID = other.ID;
        other.ID = 0;
    }
    return *this;
}

void Shader::Activate() const noexcept
{
    if (ID != 0)
        glUseProgram(ID);
}

void Shader::AttachTextureUnit(const GLuint textureUnit, const std::string& uniform)
{
	// Shader needs to be activated before changing the value of a uniform
	Activate();
    // Gets the location of the uniform
    // use signed GLint for uniform location and check against -1
	GLint loc = glGetUniformLocation(static_cast<GLint>(ID), uniform.c_str());
	// Sets the value of the uniform
	if(loc != -1) glUniform1i(loc, static_cast<GLuint>(textureUnit));
}

