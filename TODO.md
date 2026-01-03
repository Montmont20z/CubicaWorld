# High-level architecture

Minecraft-clone

Core:

C++17 (both repos use this, you're familiar with it)
Modern OpenGL 3.3+ (core profile, not compatibility)
GLFW for window/input (lightweight, standard)
GLEW or GLAD for OpenGL loading (GLAD is simpler)
GLM for math (vectors, matrices)
stb_image for texture loading (single header library)


## Realistic 1.5 month scope:
### Week 1-2: Foundation
✅ Window + Input
GLFW window
Mouse capture
Keyboard input

✅ Basic Rendering

Shader system
Render cubes
Camera matrix (view + projection)
Depth testing

### Week 3: Chunk System
✅ World Generation

16x256x16 chunks
Chunk meshing (combine cubes into single mesh)
Face culling
Simple height-based terrain (y = noise * 64)

✅ Chunk Management

Load chunks around player
Unload far chunks
Frustum culling (don't render chunks behind you)

### Week 4: Interaction
✅ Player Physics

AABB collision with blocks
Gravity
Jumping

✅ Block Editing

Voxel ray traversal (which block are you looking at?)
Break block
Place block
Update chunk mesh when blocks change

### Week 5-6: Polish
✅ Making it Feel Good

Block breaking animation/progress
Place block at correct position
Multiple block types (5-10 types)
Basic inventory/hotbar (press 1-9 to switch blocks)
Crosshair

✅ Performance

Greedy meshing (optimize mesh generation)
Multithreading chunk generation (if time)

✅ Visuals

Textures
Basic lighting (sky light + simple shading)
Fog
Sky color


Feature Priority List
Must Have (Core Gameplay):

✅ Infinite terrain
✅ Player movement + collision
✅ Break/place blocks
✅ Multiple block types
✅ Save/load world (don't lose your progress!)

Should Have (Makes it Fun):

✅ Decent performance (60+ FPS)
✅ Inventory/hotbar
✅ Basic lighting
✅ Block highlight
✅ Textures

Nice to Have (Polish):

⚠️ Water (complex!)
⚠️ Trees/structures
⚠️ Day/night cycle
⚠️ Sounds
⚠️ Particles

Don't Even Think About (Scope Creep):

❌ Mobs/enemies
❌ Crafting system
❌ Multiplayer
❌ Redstone
❌ Complex biomes
❌ Advanced lighting (shadows, AO)
❌ Shaders/post-processing

```classDiagram
    %% Core Engine
    class Game {
        -Window window
        -Renderer renderer
        -World world
        -Player player
        -Input input
        -float deltaTime
        +init()
        +run()
        +update()
        +render()
        +shutdown()
    }

    %% Window & Input
    class Window {
        -GLFWwindow* handle
        -int width
        -int height
        +create()
        +shouldClose() bool
        +swapBuffers()
        +pollEvents()
        +captureMouse()
    }

    class Input {
        -bool keys[512]
        -double mouseX, mouseY
        -double lastMouseX, lastMouseY
        +isKeyPressed(key) bool
        +getMouseDelta() vec2
        +update()
    }

    %% Rendering
    class Renderer {
        -Camera camera
        -ShaderProgram* chunkShader
        -ShaderProgram* uiShader
        +init()
        +render(World)
        +renderUI()
        +setViewport(width, height)
    }

    class Camera {
        -vec3 position
        -vec3 front
        -vec3 up
        -float yaw, pitch
        -float fov
        +getViewMatrix() mat4
        +getProjectionMatrix() mat4
        +processMouseMovement(dx, dy)
        +updateVectors()
    }

    class ShaderProgram {
        -uint programID
        -uint vertexID
        -uint fragmentID
        +load(vertPath, fragPath)
        +use()
        +setUniform(name, value)
        +compile()
        +link()
    }

    class Texture {
        -uint textureID
        -int width, height
        +load(path)
        +bind(slot)
        +unbind()
    }

    %% World & Chunks
    class World {
        -map~ivec2, Chunk*~ chunks
        -WorldGenerator generator
        -vec3 playerPos
        -int renderDistance
        +update(playerPos)
        +getBlock(x, y, z) BlockType
        +setBlock(x, y, z, type)
        +getChunk(chunkX, chunkZ) Chunk*
        +loadChunksAround(x, z)
        +unloadDistantChunks()
        +save()
        +load()
    }

    class Chunk {
        -BlockType blocks[16][256][16]
        -ivec2 position
        -Mesh mesh
        -bool isDirty
        -bool isGenerated
        +generate()
        +buildMesh()
        +getBlock(x, y, z) BlockType
        +setBlock(x, y, z, type)
        +isEmpty() bool
        +isInFrustum(frustum) bool
    }

    class Mesh {
        -uint VAO, VBO, EBO
        -vector~float~ vertices
        -vector~uint~ indices
        -int vertexCount
        +build(vertices, indices)
        +render()
        +clear()
        +destroy()
    }

    class WorldGenerator {
        -NoiseGenerator noise
        -int seed
        +generate(Chunk)
        +getHeight(x, z) int
        +getBiome(x, z) BiomeType
    }

    class NoiseGenerator {
        -int seed
        -float frequency
        -int octaves
        +noise2D(x, z) float
        +noise3D(x, y, z) float
    }

    %% Player & Physics
    class Player {
        -vec3 position
        -vec3 velocity
        -AABB boundingBox
        -Camera camera
        -Inventory inventory
        -float speed
        -float jumpForce
        -bool onGround
        +update(input, world, dt)
        +applyGravity(dt)
        +move(direction, dt)
        +jump()
        +handleCollision(world)
        +updateCamera()
    }

    class AABB {
        -vec3 min
        -vec3 max
        +intersects(other) bool
        +intersectsBlock(blockPos) bool
        +move(offset) AABB
    }

    class Inventory {
        -BlockType hotbar[9]
        -int selectedSlot
        +getSelectedBlock() BlockType
        +setSlot(index, type)
        +selectSlot(index)
    }

    %% Block System
    class BlockType {
        <<enumeration>>
        AIR
        GRASS
        DIRT
        STONE
        WOOD
        SAND
        WATER
        COBBLESTONE
        PLANKS
    }

    class BlockRegistry {
        -map~BlockType, BlockData~ blocks
        +getBlockData(type) BlockData
        +registerBlock(type, data)
    }

    class BlockData {
        -string name
        -bool isSolid
        -bool isTransparent
        -TextureCoords textures[6]
        +getTexture(face) TextureCoords
    }

    class TextureCoords {
        -float u1, v1
        -float u2, v2
    }

    %% Interaction
    class RayCast {
        -vec3 origin
        -vec3 direction
        -float maxDistance
        +cast(world) RayHitResult
        -voxelTraversal() RayHitResult
    }

    class RayHitResult {
        -bool hit
        -ivec3 blockPos
        -ivec3 normal
        -float distance
    }

    class BlockInteraction {
        -Player player
        -World world
        -RayCast rayCast
        -float breakProgress
        -ivec3 targetBlock
        +update(input, dt)
        +breakBlock()
        +placeBlock()
        +getTargetBlock() RayHitResult
    }

    %% Relationships
    Game --> Window
    Game --> Renderer
    Game --> World
    Game --> Player
    Game --> Input
    Game --> BlockInteraction

    Renderer --> Camera
    Renderer --> ShaderProgram
    Renderer --> Texture

    World --> Chunk
    World --> WorldGenerator
    World --> BlockRegistry

    Chunk --> Mesh
    Chunk --> BlockType

    WorldGenerator --> NoiseGenerator

    Player --> Camera
    Player --> AABB
    Player --> Inventory

    BlockInteraction --> Player
    BlockInteraction --> World
    BlockInteraction --> RayCast

    RayCast --> RayHitResult

    BlockRegistry --> BlockData
    BlockData --> TextureCoords
    BlockData --> BlockType

    Inventory --> BlockType
```