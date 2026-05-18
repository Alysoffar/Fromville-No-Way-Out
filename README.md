# Fromville: No Way Out

[![License](https://img.shields.io/badge/license-proprietary-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/standard-C%2B%2B20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![CMake](https://img.shields.io/badge/build-CMake-blue.svg)](https://cmake.org/)
[![OpenGL](https://img.shields.io/badge/graphics-OpenGL4.5-green.svg)](https://www.opengl.org/)

> A narrative-driven investigation adventure game exploring themes of cult manipulation, moral corruption, and supernatural forces in a mysterious town.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Requirements](#system-requirements)
- [Build Instructions](#build-instructions)
- [Project Architecture](#project-architecture)
- [Technical Specifications](#technical-specifications)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

**Fromville: No Way Out** is a morally complex investigative adventure title featuring five asymmetric protagonists uncovering a cult conspiracy spanning 9+ in-game days. Players navigate interconnected character storylines, solve contextual logic puzzles, and navigate a sophisticated moral consequence system where every decision impacts NPC relationships and story outcomes.

The game employs deferred rendering with advanced effects (SSAO, bloom post-processing), BVH-accelerated physics, dynamic environmental storytelling, and a real-time day/night cycle that influences character availability and world state.

---

## Features

### Gameplay

- **5 Playable Characters** with distinct investigation mechanics and special abilities
  - **Boyd** — Curse-fueled interrogator with rage mechanics and combat intuition
  - **Jade** — Symbol decoder with supernatural glyph vision (15s cooldown)
  - **Tabitha** — Eavesdropper with enhanced loot collection and wall-listening ability (8s cooldown)
  - **Victor** — Temporal investigator with memory mode (time slows to 20%, 30s cooldown)
  - **Sara** — Ghost-touched entity with teleportation and supernatural warnings (30s cooldown)

- **Sophisticated Moral System** — 4-choice ethical framework (Mercy/Exposure/Revenge/Sacrifice) with persistent NPC trust tracking and consequence branching

- **19 Character-Specific Logic Puzzles** — Progressive story unlocking through collaborative puzzle-solving
  - Boyd: Cipher, Connect-the-Clues, Cult Debate, Evidence Board, Ledger Rotation, Ritual Inference, Sequence Memory, Symbol Match, Word Scramble
  - Jade: Symbol Sight, Symbol Vault, Debate
  - Victor: Ghost Witness, Empty Chair, Memory, Timeline, Mirror Collapse, Silent Procession

- **Dynamic Day/Night Cycle** — Real-time progression affecting NPC routines, accessibility, and atmospheric presentation

- **Interactive Investigation** — Examine evidence, interview NPCs, uncover hidden truth through dialogue trees

- **Atmospheric Storytelling** — Environmental cues, hidden underground tunnels, and memory system for narrative cohesion

### Technical

- **Advanced Rendering Pipeline**
  - Deferred rendering with physically-based materials
  - Screen-space ambient occlusion (SSAO)
  - Bloom post-processing with multi-pass blur
  - Procedural and heightmap-based terrain
  - Dynamic skydome with atmospheric lighting
  - Real-time shadows (directional & point lights)

- **High-Performance Physics**
  - BVH-accelerated spatial queries with surface area heuristic optimization
  - Swept AABB collision detection
  - Ray casting and ground detection
  - Character-specific collision handling

- **Robust Audio System**
  - OpenAL-based spatial audio
  - Character-driven voice integration
  - Dynamic music scoring with day/night transitions
  - Ambient sound design

- **Extensible Architecture**
  - Type-safe event bus for system decoupling
  - Centralized timer management
  - Asset-driven dialogue system (JSON-based)
  - Modular puzzle framework for easy content expansion

---

## System Requirements

### Minimum Specifications

| Component | Requirement |
|-----------|-------------|
| OS | Windows 10 x64, Linux (GCC/Clang), macOS 10.15+ |
| CPU | Intel i5-8400 or equivalent |
| GPU | NVIDIA GTX 1060 / AMD RX 580 (OpenGL 4.5 support) |
| RAM | 8 GB |
| Storage | 10 GB SSD |
| Audio | OpenAL-compatible device |

### Recommended Specifications

| Component | Requirement |
|-----------|-------------|
| OS | Windows 10/11 x64 or Ubuntu 20.04 LTS |
| CPU | Intel i7-10700K or equivalent |
| GPU | NVIDIA RTX 3060 / AMD RX 6700 |
| RAM | 16 GB |
| Storage | 15 GB SSD |

### Build Requirements

- **CMake** ≥ 3.20
- **C++20 Compiler**
  - MSVC 2019+ (Windows)
  - GCC 11+ (Linux)
  - Clang 13+ (Linux/macOS)
  - MinGW w64 (Windows cross-compilation)
- **OpenGL Headers** and **Drivers** (OpenGL 4.5+)

---

## Build Instructions

### Prerequisites

Install required development tools:

**Windows (MSVC):**
```bash
# Install Visual Studio 2019 or 2022 with C++ workload
# Install CMake: https://cmake.org/download/
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install cmake g++ libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev
```

**macOS:**
```bash
brew install cmake
```

### Building from Source

```bash
# Clone repository
git clone https://github.com/studio/fromville.git
cd fromville

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --config Release --parallel $(nproc)

# Run
./fromville
```

### Build Options

```bash
# Treat warnings as errors during development
cmake -DFROMVILLE_WARNINGS_AS_ERRORS=ON ..

# Use system-installed libraries instead of FetchContent
cmake -DFROMVILLE_USE_FETCHCONTENT=OFF ..
```

### MinGW Cross-Compilation (Windows)

```bash
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

---

## Project Architecture

### Directory Structure

```
fromville/
├── src/                              # Source code (C++20)
│   ├── main.cpp                      # Application entry point
│   ├── engine/
│   │   ├── core/                     # Core systems (Engine, Window, Input, Timer)
│   │   ├── renderer/                 # 25+ specialized renderers & animation system
│   │   ├── physics/                  # BVH collision, physics queries
│   │   ├── audio/                    # OpenAL audio management
│   │   └── resources/                # Asset loading and caching
│   ├── game/
│   │   ├── Game.h/.cpp               # Central game state and character switching
│   │   ├── entities/                 # 5 playable characters + entity types
│   │   ├── world/                    # World management, day/night cycle, entity lifecycle
│   │   ├── interactions/             # Interaction nodes and dialogue system
│   │   ├── quest/                    # Quest progression and story phases
│   │   ├── story/                    # Narrative manager and story arcs
│   │   ├── puzzles/
│   │   │   ├── core/                 # PuzzleManager and base classes
│   │   │   ├── logic/                # 19 character-specific puzzle implementations
│   │   │   └── ui/                   # Puzzle UI renderers
│   │   ├── moral/                    # Moral corruption tracking and NPC trust
│   │   ├── memory/                   # Memory replay and narrative persistence
│   │   ├── ui/                       # Menu system and application states
│   │   ├── runtime/                  # Event bus, profiling, timer management
│   │   ├── symbols/                  # Symbol/glyph mechanics
│   │   ├── tunnels/                  # Underground exploration system
│   │   └── atmosphere/               # Environmental storytelling elements
├── assets/
│   ├── models/                       # 3D character and building models (OBJ/MTL)
│   ├── shaders/                      # 60+ GLSL shader programs
│   ├── dialogue/                     # Character dialogue data (JSON)
│   ├── audio/
│   │   ├── music/                    # Dynamic musical scores
│   │   ├── sfx/                      # Sound effects
│   │   └── voice/                    # Character voice lines
│   ├── config/                       # Interaction and zone definitions
│   └── textures/                     # Material textures and PBR data
├── include/                          # External headers (glad, glfw, etc.)
├── external/                         # Third-party libraries
├── CMakeLists.txt                    # CMake build configuration
├── README.md                         # This file
└── LICENSE                           # Proprietary license

```

### Core Systems

#### Engine Layer
- **Engine** — Main loop orchestration, frame timing, input polling
- **Renderer** — Deferred rendering pipeline with 25+ specialized renderers
- **Physics** — BVH-accelerated spatial queries and collision resolution
- **Audio** — Spatial audio playback and cue management
- **Input** — Context-sensitive input handling (gameplay vs. UI)

#### Game Layer
- **Character System** — 5 asymmetric protagonists with unique abilities
- **Quest System** — Parallel character quests with consequence tracking
- **Story Manager** — 4-phase narrative progression with branching outcomes
- **Moral System** — 4-choice ethical framework with NPC relationship tracking
- **Puzzle Framework** — Extensible architecture for 19 character-specific puzzles
- **Interaction System** — Node-based dialogue, items, doors, and triggers
- **World Manager** — Character switching, terrain, building transitions

#### Runtime Systems
- **Event Bus** — Type-safe pub/sub messaging for system decoupling
- **Timer Management** — Centralized frame timing and game clock
- **Performance Profiling** — Real-time performance monitoring
- **Memory System** — Persistent narrative memory across playthroughs

---

## Technical Specifications

### Graphics & Rendering

| Feature | Implementation |
|---------|-----------------|
| **API** | OpenGL 4.5 |
| **Rendering Path** | Deferred with forward transparency |
| **Lighting Model** | Physically-based materials (PBR) |
| **Effects** | SSAO, bloom, post-processing, HDR |
| **Terrain** | Procedural + heightmap-based |
| **Shadows** | Real-time directional & point (2K cascades) |
| **Animation** | Skeletal with bone transformations |
| **Text** | FreeType GPU-rendered TTF |

### Physics & Collision

| Subsystem | Details |
|-----------|---------|
| **Spatial Acceleration** | Binary Space Hierarchy (BVH) with SAH |
| **Collision Detection** | Swept AABB + static triangle mesh |
| **Raycasting** | Sub-millisecond queries on optimized BVH |
| **Terrain** | Heightmap sampling for vertical queries |
| **Character** | Special handling for multi-character collision |

### Audio

| Feature | Specification |
|---------|---------------|
| **API** | OpenAL-Soft 1.24.2 |
| **Spatial Audio** | 3D positional sound with HRTF |
| **Formats** | WAV (16-bit PCM) |
| **Channels** | Stereo + spatial effects |
| **Voice Integration** | Character-driven TTS support |

### Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| GLFW | 3.4 | Window and input management |
| GLM | 1.0.1 | Mathematics (vectors, matrices, quaternions) |
| OpenGL Loader (GLAD) | 4.5 | OpenGL function loading |
| Assimp | Latest | 3D model importing |
| OpenAL-Soft | 1.24.2 | Spatial audio rendering |
| FreeType | Latest | TrueType font rendering |
| libsndfile | Latest | WAV audio decoding |
| tinyobjloader | Embedded | OBJ/MTL parsing |
| stb_image | Embedded | Image loading (PNG, JPG, TGA) |

---

## Contributing

### Code Standards

- **Language**: C++20 with modern idioms
- **Style**: Google C++ Style Guide adapted for game development
- **Naming**: `camelCase` for variables/methods, `PascalCase` for types
- **Comments**: Doxygen-compatible documentation headers
- **Testing**: Unit tests for physics, collision, and utility systems

### Submission Process

1. Create a feature branch: `git checkout -b feature/investigation-system`
2. Implement changes with comprehensive comments
3. Verify compilation across platforms:
   ```bash
   cmake --build . --config Release
   ```
4. Submit pull request with detailed description of changes
5. Code review and testing before merge

### Performance Guidelines

- Maintain > 60 FPS on recommended hardware
- BVH queries should complete in < 1ms
- Character switches should be instantaneous
- UI rendering should not exceed 5ms per frame

---

## Acknowledgments

**Game Design & Direction**: Academic Project Team  
**Engine Architecture**: Custom C++20 Engine  
**Graphics & Rendering**: Advanced OpenGL 4.5 Pipeline  
**Physics**: BVH-accelerated collision system  
**Audio**: OpenAL-Soft spatial audio integration  

---

## License

This project is proprietary and confidential. All rights reserved.

For licensing inquiries, contact the development team.

---

## Support & Documentation

- **Issue Tracking**: GitHub Issues
- **Technical Docs**: See `docs/` directory
- **Build Logs**: Check `build-mingw/` for compilation details
- **Performance Analysis**: See `PROJECT_STARTUP_SLOW_CAUSES.txt` for optimization notes

---

**Fromville: No Way Out** © 2024-2026 — A sophisticated investigation adventure for discerning players.
