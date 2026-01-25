#pragma once
#include "Mesh.hpp"
#include "BlockType.hpp"

//     Position           Normal     Color   TexCoords
// Right (x = +0.5) - outward normal (1,0,0)
static const Vertex RIGHT_FACE[4] = {
    {{ 0.5f, 0.0f, -0.5f}, { 1,0,0}, {1,1,1}, {0,0}},
    {{ 0.5f, 1.0f, -0.5f}, { 1,0,0}, {1,1,1}, {0,1}},
    {{ 0.5f, 1.0f,  0.5f}, { 1,0,0}, {1,1,1}, {1,1}},
    {{ 0.5f, 0.0f,  0.5f}, { 1,0,0}, {1,1,1}, {1,0}},
};
// Left (x = -0.5) - outward normal (-1,0,0)
static const Vertex LEFT_FACE[4] = {
    {{-0.5f, 0.0f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, // 8
    {{-0.5f, 0.0f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, // 9
    {{-0.5f, 1.0f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, //10
    {{-0.5f, 1.0f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, //11
};

// Front (z = +0.5) - outward normal (0,0,1)
static const Vertex FRONT_FACE[4] = {
    {{-0.5f, 0.0f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, //16
    {{ 0.5f, 0.0f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, //17
    {{ 0.5f, 1.0f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, //18
    {{-0.5f, 1.0f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, //19
};
// Back (z = -0.5) - outward normal (0,0,-1)
static const Vertex BACK_FACE[4] = {
    {{-0.5f, 0.0f, -0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, //20
    {{ 0.5f, 0.0f, -0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, //21
    {{ 0.5f, 1.0f, -0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, //22
    {{-0.5f, 1.0f, -0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, //23
};

// Bottom (y = 0) - outward normal (0,-1,0)
static const Vertex BOTTOM_FACE[4] = {
    {{-0.5f, 0.0f,  0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, // 0
    {{-0.5f, 0.0f, -0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, // 1
    {{ 0.5f, 0.0f, -0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, // 2
    {{ 0.5f, 0.0f,  0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, // 3
};
// Top (y = 1) - outward normal (0,1,0)
static const Vertex TOP_FACE[4] = {
    {{ 0.5f, 1.0f,  0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, // 4
    {{ 0.5f, 1.0f, -0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, // 5
    {{-0.5f, 1.0f, -0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, // 6
    {{-0.5f, 1.0f,  0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, // 7
};
        


class Chunk {
public:
    Chunk(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, std::vector<std::unique_ptr<Texture>> &&textures, GLenum usage);
    void GenerateMesh();
    void Render(const Shader& shader, const Camera& camera, float chunkX, float chunkY, float chunkZ);

private:
    BlockType blocks_[16][256][16] = { BlockType::Dirt };
    Mesh chunkMesh_;
    std::vector<std::unique_ptr<Texture>> textures_;

};
