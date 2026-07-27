# Low-Latency Brick Breaker Engine (C++17)

A deterministic, high-performance, **zero-allocation** Brick Breaker game engine built in C++17 and Raylib.

This project re-architects standard C-style game engine patterns into **Low-Latency Systems Engineering** paradigms. It eliminates runtime dynamic memory allocation on the hot-path, ensures hardware-level cache locality via **Data-Oriented Design (DOD)**, and guarantees leak-free lifecycle management through **Move-Only RAII abstractions**.

---

## 💡 Key Architectural Highlights

### 1. Zero-Allocation Hot-Path Execution

- **Enum-Indexed `std::array` Lookups**: All active game entities, textures, sounds, and streaming audio buffers are stored in contiguous fixed-size `std::array` containers.
- **$O(1)$ Deterministic Access**: State updates and entity queries use strongly-typed `enum class` indices, bypassing map lookups or runtime pointer indirection to achieve strict $O(1)$ time complexity without dynamic heap allocation.

### 2. Strict RAII & Deterministic Resource Lifecycle

- **Move-Only Handle Wrappers**: Heavy GPU textures and audio streams are encapsulated within custom RAII handles (`TextureHandle`, `SoundHandle`, `MusicHandle`).
- **Zero-Cost Abstractions**: Move semantics (`std::move`) transfer ownership seamlessly without deep copying. Resource destruction is deterministically tied to C++ scope rules, eliminating manual `Unload*` or `Close*` calls and preventing resource leaks.

### 3. Data-Oriented Design (DOD) & Cache Locality

- Data layouts are organized sequentially to maximize L1/L2 CPU cache hit rates during collision detection and frame rendering pipelines.
- Eliminates dynamic object inheritance hierarchies (`virtual` dispatch overhead) in favor of flat value types and DOD structures to minimize branch mispredictions and pointer chasing.

### 4. Real-Time Streaming & Audio Pipeline

- **Background Audio Streaming**: Encapsulated `MusicHandle` integrated into the main loop tick, triggering $O(1)$ ring-buffer updates (`UpdateMusicStream`) without interrupting frame delivery or causing micro-stutters.

---

## 🛠️ Project Structure

The codebase strictly adheres to a decoupled, modular directory architecture:

```text
├── CMakeLists.txt          # Cross-platform CMake build configuration
├── resources/              # Static game assets
│   ├── audio/              # Sound effects (.wav) and streaming BGM
│   └── textures/           # Texture assets (.png)
└── src/
    ├── core
    │   ├── Engine.hpp      # Main engine loop and pipeline controller
    │   └── RenderPipeline.hpp
    ├── main.cpp            # Entry point
    ├── platform
    │   ├── ResourceHandle.hpp
    │   └── Window.hpp
    ├── subsystems          # Engine subsystems (ResourceManager, etc.)
    │   ├── InputManager.hpp
    │   ├── ResourceManager.hpp
    │   └── UIManager.hpp
    └── types               # RAII wrappers, configuration, and state definitions
        ├── Ball.hpp
        ├── Brick.hpp
        ├── Config.hpp
        └── Player.hpp
```

---

## 🔧 Building and Running

### Prerequisites

- **C++17 Compatible Compiler** (GCC 9+, Clang 10+, or MSVC 2019+)
- **CMake** (v3.15+)
- **Raylib** (Installed via system package manager or local install)

### Build Instructions

1. **Clone the Repository**:

```bash
git clone https://github.com/hanqpark/low-latency-brickbreaker.git
cd low-latency-brickbreaker

```

2. **Configure & Build (Using CMake & Ninja / Make)**:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

```

3. **Run the Engine**:

```bash
./BrickBreaker

```

---

## 🎯 Future Focus & Next Steps

This project served as the foundational milestone for low-latency engine architecture. The core principles developed here (RAII resource encapsulation, DOD data streams, fixed memory bounds) will be expanded into the next project: **`low-latency-dodger`**.
