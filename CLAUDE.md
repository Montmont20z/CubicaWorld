# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Instructions
Be concise, no explanations unless I ask, no repeated code.

## Project

CubicaWorld ("cubica") is a Minecraft-clone voxel renderer written in C++17 with modern OpenGL 4.3 (core profile). It is a scoped ~1.5 month learning project — see README.md for the full week-by-week feature roadmap and the explicit "don't even think about" out-of-scope list (mobs, crafting, multiplayer, redstone, biomes, shadows/post-processing).

## Build

Dependencies (GLFW, GLM) are plain system packages (Debian/Ubuntu: `libglfw3-dev`, `libglm-dev`) found via `find_package(... CONFIG REQUIRED)` against their system-installed CMake config files (`/usr/lib/.../cmake/glfw3`, `/usr/share/cmake/glm`) — no vcpkg or other package manager involved. GLAD and stb_image are vendored directly in `include/` + `src/third_party/`.

```bash
# configure (first time / after CMakeLists.txt changes)
cmake -B build

# build
cmake --build build

# run (must run from repo root — shader/asset paths in main.cpp are relative,
# e.g. "shaders/basic.vert.glsl", "assets/test.png")
./build/cubica
```

Debug builds (default, or explicit `-DCMAKE_BUILD_TYPE=Debug`) compile with AddressSanitizer enabled (see bottom of CMakeLists.txt). The VS Code launch config sets `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1` and uses gdb.

There is no test suite and no linter configured in this repo.

### Regenerating the class diagram

`diagrams/cubica_class.mmd` is generated from the source via clang-uml (config in `.clang-uml`, needs `compile_commands.json` which CMake already emits via `CMAKE_EXPORT_COMPILE_COMMANDS`):

```bash
clang-uml --generator mermaid
```

## Architecture

### Threading model: producer-consumer chunk pipeline

The central design (see `include/ChunkManager.hpp`) is a thread pool that does all heavy chunk work off the main thread, while GL calls stay strictly on the main thread:

- `ChunkManager` spawns `hardware_concurrency() - 1` worker threads on construction, each running `WorkerLoop`, which pulls `std::function<void()>` jobs off `workQueue_` (guarded by `workMutex_`/`workCV_`).
- Each `Chunk` moves through `ChunkState`: `Empty → Generated → MeshReady → Uploaded`, with `Dirty` as a re-entry point after a block edit (`SetBlock` calls `MarkDirty()`). State is an `std::atomic<ChunkState>`.
- `ChunkManager::Update()` runs once per frame on the main thread and does, in order: (1) enqueue `Generate()` + `BuildMesh()` work for newly-in-range chunks, (2) evict out-of-range chunks, (3) re-enqueue `BuildMesh()` for any `Dirty` chunks, (4) drain up to `maxUploadsPerFrame_` (default 4) entries from `uploadQueue_` calling `Chunk::UploadIfReady()` (the only place GL upload calls happen), (5) render every chunk.
- Chunks are stored as `std::shared_ptr<Chunk>` in `chunks_` (an `unordered_map<ChunkCoord, ...>`) specifically so worker-thread closures can safely hold a chunk alive after the main thread evicts/replaces the map entry — this was a deliberate fix for a prior data race (see commit `588ab9a`) and should not be changed back to `unique_ptr` without re-checking that lifetime guarantee.
- `Chunk::BuildMesh()` needs up to 4 horizontal neighbor chunks to cull border faces correctly (`ChunkNeighbors`/`GatherNeighbors()`); a null neighbor (not yet loaded) is conservatively treated as solid so faces don't pop in/out as chunks stream.
- Mutex discipline: `chunksMutex_` guards the chunk map, `uploadMutex_` guards the upload queue, `workMutex_` guards the job queue — these are separate locks and are taken in short, non-nested critical sections; keep new code following that pattern rather than introducing a single coarse lock.

### Chunk data & meshing

- Chunk block storage is a flat heap array (`std::unique_ptr<BlockType[]>`, indexed `x + y*CHUNK_SIZE*CHUNK_SIZE + z*CHUNK_SIZE`), sized `CHUNK_SIZE x CHUNK_HEIGHT x CHUNK_SIZE` (16x256x16, see `include/Constants.hpp`).
- Terrain generation (`Chunk::Generate`) is currently a flat placeholder (dirt below y=64, air above) — noise-based generation is on the roadmap but not implemented yet.
- Meshing (`Chunk::BuildMesh`) is naive per-face culling (emit a face only where the adjacent block is air), not yet greedy meshing — greedy meshing is called out in the README as a planned performance milestone.
- `MeshData` is the pure-CPU intermediate (vertices + indices) built by worker threads; `ChunkMesh` is the GPU-only counterpart (VAO/VBO/EBO) that `ChunkMesh::Upload()` populates on the main thread — never construct/touch a `ChunkMesh` off the main thread.
- `ChunkVertex` is a packed vertex format for chunk geometry (position, normal, texCoords, texLayer, ao) distinct from the generic `Vertex` used by the non-chunk `Mesh` class (used only for the placeholder cube/light meshes in `main.cpp`).

### Rendering primitives

`VAO`/`VBO`/`EBO` (in `include/`) are thin RAII wrappers around the corresponding GL objects; `VBO`/`EBO` support in-place `Update()` so repeated remeshing (e.g. after `SetBlock`) reuses the same GL buffer rather than reallocating. `Shader` loads/compiles/links GLSL from the `shaders/` directory (paths are relative to the working directory, copied next to the executable by the CMake post-build step). `Texture` currently binds single 2D textures per-mesh; a shared `TextureManager` + `GL_TEXTURE_2D_ARRAY` is a known TODO (see comment in `ChunkManager.hpp`) needed before greedy meshing can pack multiple block types into one draw call.

### Entry point

`src/main.cpp` owns the GLFW window/context setup, input polling, and the main loop (input → update → render). Per-frame it activates `chunkShader`, sets lighting uniforms, and calls `chunkManager.Update(camera.Position, chunkShader, camera)` which handles the entire chunk lifecycle described above. Non-chunk demo geometry (textured cube, light cube) is present but currently commented out.
