#include "Mesh.hpp"

Mesh::Mesh(const std::vector<Vertex> &vertices, 
           const std::vector<GLuint> &indices, 
           std::vector<std::unique_ptr<Texture>>&& textures,  // pass rvalue 
           GLenum usage = GL_STATIC_DRAW
          )
    : vertices_(vertices)
    , indices_(indices)
    , textures_(std::move(textures))
    , vbo_(vertices, usage)
    , ebo_(indices, usage)
{
    vao_.Bind();

    vao_.LinkAttrib(vbo_, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, position));
    vao_.LinkAttrib(vbo_, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    vao_.LinkAttrib(vbo_, 2, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, color));
    vao_.LinkAttrib(vbo_, 3, 2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    
    // create EBO while VAO is bound so VAO stores element array binding
    ebo_.Bind();
    
    // unbind VAO, VBO, EBO
    vbo_.Unbind();
    vao_.Unbind();
    ebo_.Unbind();
    
    // Texture
    // Texture popCat("assets/pop-cat.jpg", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
    // popCat.texUnit(shader, "tex0", 0);
}

Mesh::~Mesh() noexcept
{
}

void Mesh::Draw(const Shader &shader, const Camera &camare) const
{
    shader.Activate();
    vao_.Bind();

    for (size_t i = 0; i < textures_.size(); ++i){
       textures_[i]->Bind(i); 
    }
     
    // draw the mesh
    glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, (void*)0); 
}
