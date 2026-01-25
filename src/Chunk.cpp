#include "Chunk.hpp"
#include "Constants.hpp"
#include <iostream>

Chunk::Chunk(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, std::vector<std::unique_ptr<Texture>>&& textures, GLenum usage = GL_STATIC_DRAW)
    : textures_(std::move(textures))
    , chunkMesh_(vertices, indices, std::vector<std::unique_ptr<Texture>>{}, usage)
{
    // initialize blocks chunk
    for (int x = 0; x < CHUNK_SIZE; ++x)
        for (int y = 0; y < 64; ++y)
            for (int z = 0; z < CHUNK_SIZE; ++z)
                blocks_[x][y][z] = BlockType::Dirt;
    // auto* first = &blocks_[0][0][0];
    // std::fill(first, first + CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE, BlockType::Dirt);
}


void Chunk::GenerateMesh()
{

    // isAir helper, check if neigbour isAir
    auto isAir = [&](int x, int y, int z)->bool{
        if (x < 0 || x >= CHUNK_SIZE ||
            y < 0 || y >= CHUNK_HEIGHT || 
            z < 0 || z >= CHUNK_SIZE)
                return true;
        return blocks_[x][y][z] == BlockType::Air;
    };
    
    // append face helper, if neighbour is air
    auto appendFace = [&](const Vertex face[4], int bx, int by, int bz, std::vector<Vertex>& verts, std::vector<GLuint>& idxs)->void{
        size_t base = verts.size();
        
        // insert vertex to verts
        for (int i = 0; i < 4; i++){
            Vertex v = face[i];
            // translate template vertex into block position
            // generate in chunk-local space and use model matrix to draw chunk
            v.position += glm::vec3(static_cast<float>(bx),static_cast<float>(by),static_cast<float>(bz));
            
            verts.push_back(v);
        }
        // two triangles (CCW), offset by base
        idxs.insert(idxs.end(), 
            // triangle 1
            {static_cast<GLuint>(base + 0),
            static_cast<GLuint>(base + 1),
            static_cast<GLuint>(base + 2),
            // triangle 2
            static_cast<GLuint>(base + 2),
            static_cast<GLuint>(base + 3),
            static_cast<GLuint>(base + 0)}
        );
    };


    
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    // std::vector<std::unique_ptr<Texture>> emptyTextures; // No textures for light
    
    // for every block in chunk
    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int y = 0; y < CHUNK_HEIGHT; y++){
            for (int z = 0; z < CHUNK_SIZE; z++){
                // if the block is air, do nothing
                if (blocks_[x][y][z] == BlockType::Air) continue;
                
                // if the block is not air 
                // test its 6 neighbour is air or out of bound
                // if air or out of bound, add that face to the mesh to render later
                if (isAir(x+1,y,z)) appendFace(RIGHT_FACE,x,y,z, vertices, indices); 
                if (isAir(x-1,y,z)) appendFace(LEFT_FACE,x,y,z, vertices, indices); 
                if (isAir(x,y+1,z)) appendFace(TOP_FACE,x,y,z, vertices, indices); 
                if (isAir(x,y-1,z)) appendFace(BOTTOM_FACE,x,y,z, vertices, indices); 
                if (isAir(x,y,z+1)) appendFace(FRONT_FACE,x,y,z, vertices, indices); 
                if (isAir(x,y,z-1)) appendFace(BACK_FACE,x,y,z, vertices, indices); 
            }
        }
    }

    // upload buffers to Mesh
    if (!vertices.empty() || !indices.empty()){
        chunkMesh_  = Mesh(vertices, indices, std::move(textures_), GL_STATIC_DRAW);
    }
}

void Chunk::Render(const Shader& shader, const Camera& camera, float chunkX, float chunkY, float chunkZ)
{
    if (chunkMesh_.IndexCount() == 0) return; // skip if no index to draw

    shader.Activate();
    // Translate by chunk * CHUNK_SIZE (assuming CHUNK_SIZE = 16 and CHUNK_HEIGHT = 256)
    const float CHUNK_SIZE_F = static_cast<float>(CHUNK_SIZE);      // e.g. 16
    const float CHUNK_HEIGHT_F = static_cast<float>(CHUNK_HEIGHT);  // e.g. 256

    glm::vec3 worldOffset = glm::vec3(
        chunkX * CHUNK_SIZE_F,
        chunkY * CHUNK_HEIGHT_F,
        chunkZ * CHUNK_SIZE_F
    );

    // set model matrix to put local coordinates into world coordinates
    glm::mat4 model = glm::translate(glm::mat4(1.0f), worldOffset);
    shader.SetMat4("model", model);
    
    // set camera
    // camera.UpdateMatrix(45.0f, 0.1f, 100.0f);
    camera.SetUniformMatrix(shader, "camMatrix");
    
    // draw the mesh once per chunk
    chunkMesh_.Draw(shader, camera);
}