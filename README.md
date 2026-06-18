# Game Template

A bare C++ 2D game starter. It boots to an empty window and gives you the
reusable engine plumbing to build on — no game-specific content included.

## What's included

- **Window / input** — GLFW + OpenGL 3.3 core context, keyboard and mouse callbacks.
- **ECS** — a tiny entity-component-system (`tiny_ecs`).
- **Rendering** — a plain 2D pipeline: textured sprites, colored meshes, sprite-sheet
  animation, and instanced particles, drawn through an orthographic camera.
- **Physics** — SAT collision detection and impulse resolution for convex rigid bodies,
  with static (`Wall`) and dynamic (`MoveableWall`) sample bodies.
- **Level loading** — a data-driven JSON loader (`LevelLoader`) that maps level-file
  keys to spawner functions you register.
- **UI** — reusable clickable `Button`, `PopUP` dialog, `HighLightCircle`, and a
  fullscreen `MainMenu` helper.
- **Audio** — SDL_mixer is initialized and ready; no audio assets are bundled.

The starting scene (`data/levels/template.json`) is intentionally empty, so the app
opens to a cleared window. Press `O` to toggle debug draw, `P` for profiling output,
and `R` to reload the current level.

## Build

Use CMake to configure and build the project. On Windows, the repo includes the
prebuilt GLFW and SDL libraries expected by `CMakeLists.txt`.

```powershell
cmake -S . -B build
cmake --build build
```

The build copies `data/` beside the executable after compilation.

## Project layout

- `src/core`: entry point and main loop, ECS, shared math/components, global state, debug draw.
- `src/rendering`: OpenGL rendering, camera, render components, particles.
- `src/gameplay/systems`: world loop, physics, JSON level loader.
- `src/gameplay/entities`: sample physics bodies (`Wall`, `MoveableWall`).
- `src/ui`: reusable button / popup / menu UI components.
- `data/levels`: JSON scenes (`template.json` is the empty starting scene).
- `data/shaders`: the generic shader programs used by the renderer.
- `data/textures`, `data/audio`: add your game's assets here (empty by default).

## Starting a new game

1. Rename the CMake project (`project(game_template)`) if you want a different executable name.
2. Add your art to `data/textures` and sounds to `data/audio`.
3. Define your own entities/components and systems under `src/gameplay`.
4. Register spawners in `LevelLoader::level_objects` (keyed by a level-file type name)
   and author scenes as JSON under `data/levels`, then start one from `main.cpp`
   (`start_level`).
5. Expand `GameInstance` only with state that must be shared across systems.
