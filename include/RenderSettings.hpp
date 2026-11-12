constexpr bool IS_SINGLE_THREADED = false;
constexpr bool IS_FRUSTUM_CULLING = true;
constexpr bool IS_HORIZON_CULLING = true;
constexpr int VIEW_DISTANCE = 16; 

enum class MeshingMode { Naive, NaiveCulled, Greedy }; // naive exclude face culled 
constexpr MeshingMode MESHING_MODE = MeshingMode::Greedy;