#include "ChunkMesh.hpp"
#include "EBO.hpp"
#include "MeshData.hpp"
#include <cstddef>
#include <memory>
// #include <iostream>

void ChunkMesh::Upload(MeshData&& data)
{
    indexCount_ = static_cast<GLuint>(data.indices.size());
    if (indexCount_ == 0) { vbo_.reset(); ebo_.reset(); data.Clear(); return;  } 

    if(!vbo_){
        // First upload: create buffers, link attribues once.
        vbo_ = std::make_unique<VBO<ChunkVertex>>(data.vertices, GL_DYNAMIC_DRAW);
        ebo_ = std::make_unique<EBO>(data.indices,GL_DYNAMIC_DRAW);
        
        vao_.Bind();
        // NOTE: requires VAO::LinkAttrib to take a raw GLuint buffer ID (see VAO patch)
        // and an isInteger flag that routes to glVertexAttribIPointer for texLayer/ao.
        vao_.LinkAttrib(vbo_->ID, 0, 3, GL_FLOAT, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, position));
        vao_.LinkAttrib(vbo_->ID, 1, 3, GL_FLOAT, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, normal));
        vao_.LinkAttrib(vbo_->ID, 2, 2, GL_FLOAT, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, texCoords));
        vao_.LinkAttrib(vbo_->ID, 3, 1, GL_UNSIGNED_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, texLayer),true);
        vao_.LinkAttrib(vbo_->ID, 4, 1, GL_UNSIGNED_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, ao), true);
        
        // create EBO while VAO is bound so VAO stores element array binding
        ebo_->Bind();
        // Debug  
        // GLint boundVAO = 0;
        // GLint boundEBO = 0;

        // glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVAO);
        // glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &boundEBO);

        // std::cerr
        //     << "[Upload] VAO=" << boundVAO
        //     << ", EBO=" << boundEBO
        //     << ", isBuffer="
        //     << static_cast<int>(glIsBuffer(
        //         static_cast<GLuint>(boundEBO)))
        //     << '\n';

        vao_.Unbind();
        

    } else {
        // Remesh path: same buffer objects, no VAO rebuild.
        vbo_->Update(data.vertices);
        ebo_->Update(data.indices);
    }
    data.Clear();
}

void ChunkMesh::Draw() const {
    if (indexCount_ == 0) return;
    vao_.Bind();

    // Debug
    // GLint boundVAO = 0; GLint boundEBO = 0;
    // glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVAO);
    // glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &boundEBO);

    // if (boundVAO == 0 ||
    //     !glIsVertexArray(static_cast<GLuint>(boundVAO)))
    // {
    //     std::cerr << "Invalid VAO: " << boundVAO << '\n';
    //     return;
    // }

    // if (boundEBO == 0 ||
    //     !glIsBuffer(static_cast<GLuint>(boundEBO)))
    // {
    //     std::cerr
    //         << "Invalid EBO: " << boundEBO
    //         << ", VAO: " << boundVAO << '\n';
    //     return;
    // }

    // GLint eboBytes = 0;

    // glGetBufferParameteriv(
    //     GL_ELEMENT_ARRAY_BUFFER,
    //     GL_BUFFER_SIZE,
    //     &eboBytes
    // );

    // const auto requiredBytes =
    //     static_cast<std::uint64_t>(indexCount_) *
    //     sizeof(GLuint);

    // if (requiredBytes > static_cast<std::uint64_t>(eboBytes))
    // {
    //     std::cerr
    //         << "Index buffer too small: required = "
    //         << requiredBytes
    //         << ", actual = " << eboBytes << '\n';
    //     return;
    // }


    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}