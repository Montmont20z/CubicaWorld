# CubicaWorld — Voxel Game Engine
A voxel-based rendering engine built from scratch in C++17 and OpenGL, investigating how real-time performance can be sustained in an infinite, dynamically generated, cube-based 3D world.

## Main Goal
Showcase performance optimization method in a voxel game engine
 
1. Excessive vertex count from un-merged adjacent same-type faces → **greedy meshing**
2. Wasted GPU submission for geometry outside the camera's view → **frustum culling**
3. Overdraw from chunks hidden behind other opaque geometry → **occlusion culling**


## Tech Stack
C++17
OpenGL 4.3 (core profile)
GLFW — window/input
GLAD — OpenGL function loading
GLM — vector/matrix math
stb_image — texture loading
GLSL — custom shaders
CMake — build system
clang-uml — UML class diagram generation from source

# Installation 
```bash
sudo apt install libglfw3-dev
sudo apt install libglm-dev
```

# Common Commands
```bash
# regenerate mermaid diagram 
clang-uml --generator mermaid
```

## Scope

### Included
Chunk-based voxel world (16×256×16 blocks/chunk) with neighbour-aware meshing
Multithreaded chunk generation and mesh processing
Greedy meshing, frustum culling, occlusion culling
Procedural terrain via multi-octave Perlin noise (configurable seed)
Voxel ray traversal for block place/break with dynamic mesh updates
First-person movement, AABB collision, gravity, jumping
GLSL pipeline: texture mapping, UV atlasing, per-face shading
Before/after benchmarking per optimisation technique

### Excluded
Multiplayer/networking
Mobs/NPCs, AI-driven entities
Crafting/item progression
Redstone-like logic
Advanced global illumination, dynamic shadows, post-processing
Complex biomes
Audio

## Current Status

### Implemented and verified:
GLFW window + input handling
Free-fly camera with mouse look
RAII wrappers: Shader, Texture, VAO, VBO, EBO
Mesh class tying buffers together for drawing
GLSL pipeline with texture mapping; Phong-style point/directional/spot lighting functions written (active render path currently uses simple textured shading)
BlockType enum (Air, Grass, Dirt, Stone)
Chunk class (16×256×16 array) with neighbour-based face-culled mesh generation

`Neighbour-based culling omits faces adjacent to solid blocks but is distinct from full greedy meshing — faces are still emitted per block, not merged across neighbours.`

### Not yet implemented:
Greedy face merging
Frustum culling
Occlusion culling
Multi-chunk world management/streaming
Procedural terrain generation
Player physics and collision
Block placement/removal via ray casting

## Project Schedule
 
| Phase | Status | Date |
|---|---|---|
| Window/context setup, input, first test cube | ✅ Completed | 3–12 Jan 2026 |
| Free-fly camera, RAII refactor, initial UML | ✅ Completed | 13–17 Jan 2026 |
| Mesh class, texture loading, lighting/fog shaders | ✅ Completed | 18–21 Jan 2026 |
| BlockType enum, Chunk class, face-culled meshing | ✅ Completed | 22–26 Jan 2026 |
| README/UML regenerated | ✅ Completed | 5–8 Jun 2026 |
| Greedy meshing | ⏳ Planned | 22 Jun – 12 Jul 2026 |
| Frustum culling | ⏳ Planned | 13–26 Jul 2026 |
| Occlusion culling | ⏳ Planned | 27 Jul – 16 Aug 2026 |
| Procedural terrain (Perlin noise) | ⏳ Planned | 17 Aug – 6 Sep 2026 |
| Player physics & interaction | ⏳ Planned | 7–27 Sep 2026 |
| Benchmarking & report | ⏳ Planned | 28 Sep – 18 Oct 2026 |
 
 
## Performance Targets
 
| Metric | Target |
|---|---|
| Sustained frame rate | 60+ FPS at full render distance |
| Render distance | 8+ chunks (≈128 blocks) |
| Draw call reduction | ≥80% (greedy meshing vs. naive baseline) |