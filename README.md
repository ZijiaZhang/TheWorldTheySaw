# Game Template

This repository is a reusable C++ game starter built around GLFW, OpenGL, SDL_mixer, a tiny ECS, JSON level loading, rendering helpers, simple physics, and AI/pathing examples.

The old game's campaign data has been reduced to a neutral starter setup:

- `data/levels/menu.json` is a minimal menu scene.
- `data/levels/level_1.json` is a small playable sample level.
- `data/levels/settings.json`, `win.json`, and `lose.json` are utility screens retained for the existing loop.
- The previous game's authored level files and balance notes have been removed.

## Build

Use CMake to configure and build the project. On Windows, the repo includes the prebuilt GLFW and SDL libraries expected by `CMakeLists.txt`.

```powershell
cmake -S . -B build
cmake --build build
```

The build copies `data/` beside the executable after compilation.

## Template Map

- `src/core`: entry point, common math/path helpers, ECS, and global game state.
- `src/rendering`: OpenGL rendering, shader setup, camera, backgrounds, particles, and visual components.
- `src/gameplay/systems`: world loop, physics, level loading, timers.
- `src/gameplay/entities`: sample entity implementations.
- `src/gameplay/ai`: sample AI, pathing, and weapon configuration.
- `src/ui`: reusable button/loading/menu UI components.
- `data/levels`: JSON scenes that instantiate registered level objects.
- `data/shaders`: shader programs used by the renderer.
- `data/textures`, `data/audio`, `data/meshes`: assets to replace with your new game's content.

## Starting A New Game

1. Rename the CMake project if you want a game-specific executable name.
2. Replace the sample assets in `data/textures` and `data/audio`.
3. Add or rename level object types in `LevelLoader::level_objects`.
4. Replace the sample player/enemy/weapon entities with your new game's components and systems.
5. Expand `GameInstance` only with state that must be shared across systems.
