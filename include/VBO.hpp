#pragma once
#include <glad/glad.h>
#include <vector>
#include <stdexcept>

// Templated because the buffer logic (gen/bind/upload/delete) is identical
// for any vertex type — this is the real DRY case, unlike Mesh/ChunkMesh
// which differ in attribute layout, not buffer logic.
template <typename VertexT>
class VBO {
public:
    GLuint ID{0};
    explicit VBO(const std::vector<VertexT> &vertices, GLenum usage)
        : byteSize_(vertices.size() * sizeof(VertexT))
        , usage_(usage)
    {
        glGenBuffers(1, &ID); 
        if (ID == 0) {
            throw std::runtime_error("VBO: glGenBuffers returned 0\n");
            return;
        }
        // upload vertex data (to GPU)
        glBindBuffer(GL_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
        glBufferData(GL_ARRAY_BUFFER, byteSize_, vertices.data(), usage); // GL_STATIC_DRAW hints that data will not change frequenctly
    }
    ~VBO() noexcept { if(ID) glDeleteBuffers(1, &ID); };
    
    // Delete Copy
    VBO(const VBO&) = delete;
    VBO& operator=(const VBO&) = delete;

    // Move
    VBO(VBO&& other) noexcept 
        : ID(other.ID)
        , byteSize_(other.byteSize_)
        , usage_(other.usage_)
    { other.ID = 0; };

    VBO& operator=(VBO&& other) noexcept
    {
        if (this == &other) return *this;
        if (this->ID != 0){
           glDeleteBuffers(1, &ID);
        }
        // steal other properties
        this->ID = other.ID;
        this->byteSize_ = other.byteSize_;
        this->usage_ = other.usage_;
        
        // clean other 
        other.ID = 0;
        other.byteSize_ = 0;
        other.usage_ = 0;
        return *this;
    };

    void Bind() const {
        // upload vertex data (to GPU)
        glBindBuffer(GL_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
    };
    void Unbind() const{
        // unbind VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0); 
    };
    
    // Re-uploads into the SAME buffer object - no VAO rebuild needed.
    // use for chunk remesh instead of destroying & recreating the Mesh/ChunkMesh
    void Update(const std::vector<VertexT>& vertices){
        byteSize_ = vertices.size() * sizeof(VertexT); 
        // upload vertex data (to GPU)
        glBindBuffer(GL_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
        glBufferData(GL_ARRAY_BUFFER, byteSize_, vertices.data(), usage_); // GL_STATIC_DRAW hints that data will not change frequenctly
    }
private:
    std::size_t byteSize_{0};
    GLenum usage_{GL_STATIC_DRAW};
};