# High-level architecture
Minecraft-clone

Core:
C++17 (both repos use this, you're familiar with it)
Modern OpenGL 4.3+ (core profile, not compatibility)
GLFW for window/input (lightweight, standard)
GLEW or GLAD for OpenGL loading (GLAD is simpler)
GLM for math (vectors, matrices)
stb_image for texture loading (single header library)

# Common Commands
```bash
# regenerate mermaid diagram 
clang-uml --generator mermaid
```


## Realistic 1.5 month scope:
### Week 1-2: Foundation
#### Window + Input
✅ GLFW window
Mouse capture
Keyboard input
Camera movement

#### Basic Rendering

✅ Shader system
Render cubes
Camera matrix (view + projection)
Depth testing

### Week 3: Chunk System
#### World Generation

16x256x16 chunks
Chunk meshing (combine cubes into single mesh)
Face culling
Simple height-based terrain (y = noise * 64)

#### Chunk Management

Load chunks around player
Unload far chunks
Frustum culling (don't render chunks behind you)

### Week 4: Interaction
#### Player Physics

AABB collision with blocks
Gravity
Jumping

 #### Block Editing

Voxel ray traversal (which block are you looking at?)
Break block
Place block
Update chunk mesh when blocks change

### Week 5-6: Polish
####  Making it Feel Good

Block breaking animation/progress
Place block at correct position
Multiple block types (5-10 types)
Crosshair

#### Performance

Greedy meshing (optimize mesh generation)
Multithreading chunk generation 

#### Visuals

Textures
Basic lighting (sky light + simple shading)
Fog
Sky color


### Feature For Gameplay List
#### Must Have (Core Gameplay):

✅ Infinite terrain
✅ Player movement + collision
✅ Break/place blocks
✅ Multiple block types

✅ Decent performance (60+ FPS)
✅ Basic lighting
✅ Textures

### Most important technical demo features
1. Rendering Performance Optimization
- Greedy meshing algorithm 
    > Combine adjeacent voxel to reduce draw call
- Frustum culling 
    > dont renderer chunck outside camera view
- Occlusion culling
    > dont render chunck behind other chunks
- Level of details 
    > Reduce detial for distance chunck
2. Chuck Management
- Infinite world generation
- Multithreading
    > Generate mesh & chunck on background threads
- Memory management
    > Pool chunck, smart caching
3. Procedural generation
- Perlin/Simplex noise for terrain
4. Others
- Deferred rendering or forward rendering
- Ambient occlusion for visual quality

### Document these
1. Performance metrics
    - eg render 100,000 visible faces at 60 FPS
    - reduce draw call by 90% using greedy meshing algo
    - before and after optimization graph
2. Technical decision
    - why specific algorithm 
    - trade off considered
    - problem solve
3. code quality 
    - architecture
    - well comment complex section
    - unit test for critical section
4. Video

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