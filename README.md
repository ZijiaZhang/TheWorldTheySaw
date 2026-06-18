# Game Template

A C++ 2D game starter with a full **dynamic deferred lighting** pipeline. It boots
to a small lit demo scene and gives you the reusable engine plumbing to build on.

## What's included

- **Window / input** — GLFW + OpenGL 3.3 core context, keyboard and mouse callbacks.
- **ECS** — a tiny entity-component-system (`tiny_ecs`).
- **Deferred lighting** — a pseudo-3D pipeline (see below): G-buffer, jump-flooding
  SDF, Radiance Cascades global illumination, a normal-mapped spotlight with
  height-field shadows, and an ACES composite.
- **Forward rendering** — textured sprites, colored meshes, sprite-sheet animation,
  and instanced particles, used here for UI overlays on top of the lit scene.
- **Physics** — SAT collision detection and impulse resolution for convex rigid bodies,
  with static (`Wall`) and dynamic (`MoveableWall`) sample bodies.
- **Level loading** — a data-driven JSON loader (`LevelLoader`) that maps level-file
  keys to spawner functions you register.
- **UI** — reusable clickable `Button`, `PopUP` dialog, `HighLightCircle`, and a
  fullscreen `MainMenu` helper.
- **Audio** — SDL_mixer is initialized and ready; no audio assets are bundled.

The app boots into a procedurally-generated lighting demo (`setupLightingDemo`): a
floor, normal-mapped "stud" occluders and tall walls at different heights, and an
emissive blob. A **flashlight follows the mouse** and casts 3D-feel height shadows;
the emissive blob feeds soft global illumination. No binary art assets are needed —
every texture is generated at runtime.

Controls: move the mouse to aim the light; `R` reloads the scene; `O` toggles debug
draw; `P` toggles profiling; **number keys `0`–`8` switch the composite debug view**
(0 = final, 1 = albedo, 2 = normal, 3 = height, 4 = SDF, 5 = GI, 6 = direct light,
7 = emissive, 8 = occluder mask).

## Build

Use CMake to configure and build the project. On Windows, the repo includes the
prebuilt GLFW and SDL libraries expected by `CMakeLists.txt`.

```powershell
cmake -S . -B build
cmake --build build
```

The build copies `data/` beside the executable after compilation.

## Dynamic deferred lighting

The lighting pipeline implements the design in `动态光照系统设计文件.md`; the exact
integration contract (G-buffer formats, texture units, uniform names, pass order)
lives in `LIGHTING_DESIGN_CONTRACT.md`. It runs entirely as full-screen fragment
passes on OpenGL 3.3 core (no compute shaders), with ping-pong FBOs for the
iterative passes. Per frame:

1. **Geometry → G-buffer** — `LitSprite` entities write albedo+occluder-mask,
   encoded normal, height, and emissive into a 4-target MRT.
2. **JFA SDF** — a jump-flooding pass turns the occluder mask into a signed
   distance field (`data/shaders/jfa_*`, `sdf_distance`).
3. **Radiance Cascades GI** — multi-cascade radiance probes sphere-trace the SDF to
   gather emissive light, merged top-down with the Osborne–Sannikov bilinear fix
   (`rc_cascade`, `rc_merge`).
4. **Direct light + shadows** — a deferred spotlight uses the G-buffer normal, with a
   height-field ray-march that produces 3D-feel shadows whose length scales with
   occluder height and light elevation (`direct_light`).
5. **Composite** — `albedo * (gi + direct) + emissive`, then ACES tonemap
   (`composite`).

Author lit content by giving an entity a `Motion` + a `LitSprite` (in
`src/rendering/deferred_lighting.hpp`): supply albedo/normal/height/emissive
textures, a `baseHeight`, and `isOccluder` (note: **emitters must be occluders** so
GI rays can hit and gather them). The single hero flashlight lives in
`RenderSystem::deferred.lights.flashlight` and is aimed from the mouse each frame in
`RenderSystem::draw`.

> RC note: to keep every cascade in one `W×H` texture on GL 3.3, the cascade packing
> uses `4^c` directions per probe (configured via `baseDirCount = 1`), which trades
> some angular resolution at the coarsest cascade for simplicity. GI is functional
> and soft but lower-frequency than a full-resolution multi-texture RC; raise
> `RadianceCascades::Params` (more cascades / directions, a 2×-size cascade texture)
> for higher fidelity.

## Project layout

- `src/core`: entry point and main loop, ECS, shared math/components, global state, debug draw.
- `src/rendering`: forward renderer, camera, render components, particles, the deferred
  lighting pipeline (`deferred_lighting.*`), and the lighting demo (`demo_scene.*`).
- `src/gameplay/systems`: world loop, physics, JSON level loader.
- `src/gameplay/entities`: sample physics bodies (`Wall`, `MoveableWall`).
- `src/ui`: reusable button / popup / menu UI components.
- `data/levels`: JSON scenes (`template.json` is the empty starting scene).
- `data/shaders`: forward shaders plus the deferred-lighting passes
  (`fullscreen`, `gbuffer`, `jfa_*`, `sdf_distance`, `rc_cascade`, `rc_merge`,
  `direct_light`, `composite`).
- `data/textures`, `data/audio`: add your game's assets here (empty by default).
- `LIGHTING_DESIGN_CONTRACT.md`: the authoritative spec for the lighting pipeline.

## Starting a new game

1. Rename the CMake project (`project(game_template)`) if you want a different executable name.
2. Add your art to `data/textures` and sounds to `data/audio`.
3. Define your own entities/components and systems under `src/gameplay`.
4. Register spawners in `LevelLoader::level_objects` (keyed by a level-file type name)
   and author scenes as JSON under `data/levels`, then start one from `main.cpp`
   (`start_level`).
5. Expand `GameInstance` only with state that must be shared across systems.
6. To start from a blank scene instead of the lighting demo, remove the
   `WorldSystem::post_restart = setupLightingDemo;` line (and its include) in `main.cpp`;
   spawn your own `LitSprite` entities to light, and tune the flashlight via
   `RenderSystem::deferred.lights.flashlight`.
