# Pixel

**A 2D game engine in C++** built with an entity-component-system (ECS) architecture, SDL2, and a data-oriented design. Suitable for 2D games, prototypes, and learning low-level game engine concepts.

---

## Purpose

Pixel is a custom 2D game engine that demonstrates:

- **ECS architecture** — Entities, component pools, and systems with signature-based matching
- **Event-driven design** — Decoupled communication via an event bus (e.g. collisions, input)
- **Asset management** — Central store for textures and fonts with SDL2
- **Modular layout** — Clear separation between Game, ECS, Logger, Events, and AssetStore

It serves as a portfolio project showing systems programming, C++17, and game engine design without relying on a full engine like Unity or Unreal.

---

## Features

- **Entity–Component–System**
  - Entity creation/destruction, tags, and groups
  - Component pools with stable indices and reuse
  - Systems that subscribe to component signatures and run each frame

- **Rendering**
  - Sprite rendering with z-order and camera
  - Tilemap loading from map files
  - Text labels and health bars
  - Optional debug overlay (collision boxes, ImGui)

- **Gameplay**
  - Movement and keyboard-controlled entities
  - Camera follow
  - AABB collision detection and collision events
  - Projectile emission and lifecycle
  - Health and damage

- **Tooling**
  - ImGui debug panel (map coordinates, spawn enemies with parameters)
  - Logger (info, error, warn) with timestamps
  - Event bus for input and collision events

- **Demo**
  - Sample level: tilemap, player chopper, enemies (tank, truck), obstacles, projectiles, and UI

---

## Installation

### Requirements

- **C++17** compiler (e.g. GCC 8+, Clang 6+, or MSVC 2017+)
- **SDL2** (and development headers)
- **SDL2_image**, **SDL2_ttf**, **SDL2_mixer**
- **CMake** is optional; the project uses a Makefile by default

### Dependencies (examples)

**macOS (Homebrew):**

```bash
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer
```

**Ubuntu / Debian:**

```bash
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

**Fedora:**

```bash
sudo dnf install SDL2-devel SDL2_image-devel SDL2_ttf-devel SDL2_mixer-devel
```

### Build

From the project root:

```bash
cd pixel
make build
```

To use a specific compiler (e.g. GCC 14), set `CC`:

```bash
make build CC=g++-14
```

### Run

```bash
make run
# or
./pixel
```

The binary must be run from the project root (or the directory containing the `assets/` folder) so that asset paths like `./assets/images/` and `./assets/tilemaps/` resolve correctly.

---

## Usage

- **Arrow keys** — Move the player chopper
- **Z** — Stop movement
- **Space** — Fire projectiles (player only)
- **D** — Toggle debug mode (collision boxes + ImGui panel)
- **Escape** — Quit

In debug mode you can use the ImGui panel to view map coordinates and spawn enemy entities with custom transform, velocity, projectile, and health settings.

---

## Project Structure

```
pixel/
├── assets/              # Fonts, images, tilemaps (required at runtime)
│   ├── fonts/
│   ├── images/
│   └── tilemaps/
├── libs/                 # Third-party (imgui, glm, etc.)
├── src/
│   ├── Main.cpp
│   ├── Components.h      # ECS component definitions
│   ├── Systems.h         # ECS system implementations
│   ├── Game/
│   │   ├── Game.h
│   │   └── Game.cpp      # Window, loop, LoadLevel, input
│   ├── ECS/
│   │   ├── ECS.h        # Entity, World, System, Pool
│   │   └── ECS.cpp
│   ├── Logger/
│   ├── Events/          # Event, EventBus, CollisionEvent, KeyPressedEvent
│   └── AssetStore/
├── Makefile
├── LICENSE
└── README.md
```

---

## Technologies

| Area         | Choice                                     |
| ------------ | ------------------------------------------ |
| Language     | C++17                                      |
| Graphics     | SDL2 (window, renderer, image, TTF, mixer) |
| Math         | GLM                                        |
| Debug UI     | Dear ImGui + ImGuiSDL                      |
| Architecture | ECS (custom), event bus                    |

---

## Suggested Improvements & Extensions

- **Fixed timestep** — Use a fixed update step (e.g. 60 ticks/s) with interpolation for rendering to keep physics stable and deterministic.
- **Scripting** — Integrate Lua (e.g. via Sol2, already in `libs/`) for level logic or entity behavior without recompiling.
- **Sound** — Use SDL2_mixer in the demo (e.g. shoot, hit, music) and expose volume in a simple API.
- **Persistence** — Save/load level or high scores (e.g. JSON or binary format).
- **More systems** — Input mapping, simple AI (e.g. follow/patrol), particle effects, or sprite batching for performance.
- **Build** — Add a CMakeLists.txt for cross-platform builds and optional Lua/ImGui toggles.
- **Tests** — Unit tests for ECS (create/destroy entities, add/remove components, system queries) and collision math.

---

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.
