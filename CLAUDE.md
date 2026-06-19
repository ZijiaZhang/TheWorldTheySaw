# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A small C++17 2D sprite-game engine (GLFW + SDL_mixer + glm + gl3w + stb_image +
nlohmann_json), started from a course game template. The `LightingResearch` branch
adds a substantial **pseudo-3D oblique deferred lighting system** (G-buffer →
JFA SDF → Radiance Cascades GI → spotlight + height-field shadows → composite) on
top of the otherwise-forward renderer. See [docs/LIGHTING.md](docs/LIGHTING.md).

## Build & run

Windows / MSVC is the primary toolchain (Linux/macOS paths exist in CMake but the
prebuilt GLFW & SDL libraries/DLLs under `ext/` are Windows-only).

```powershell
cmake -S . -B build              # configure (re-run after adding files/include dirs)
cmake --build build --config Debug
```

- Requires a Visual Studio "x64 Native Tools" environment (or run from the VS
  Developer prompt) so `cl`/`cmake` find MSVC.
- `add_custom_command(POST_BUILD ...)` copies the whole `data/` tree next to the
  exe after every build. **The app loads `data/` relative to its working
  directory**, so run with cwd = the exe folder: `build/Debug/game_template.exe`
  from `build/Debug`. Editing a shader/asset under `data/` only takes effect after
  a rebuild (which re-runs the copy).
- Sources are globbed with `GLOB_RECURSE ... CONFIGURE_DEPENDS` (`src/*.cpp|*.hpp`),
  so new files are picked up automatically — but **adding a new include directory
  requires editing `CMakeLists.txt` and re-running `cmake -S . -B build`**.
- Build trees in the repo: `build/` (Visual Studio generator, used for the commands
  above) and `cmake-build-debug/` (Ninja/CLion). Both are gitignored.

### No test suite

There are no automated tests or linter. Verify changes by **building and running**:
the renderer calls `gl_has_errors()` (in `render.cpp`) after most GL operations,
which **throws `std::runtime_error` on any GL error**, so an uncaught throw aborts
the process — a clean multi-second run with empty stderr means shaders compiled,
FBOs are complete, and every pass executed. Quick smoke test:

```powershell
$wd = Resolve-Path "build\Debug"
$p = Start-Process (Join-Path $wd game_template.exe) -WorkingDirectory $wd -PassThru `
     -RedirectStandardError err.txt
Start-Sleep 5; if (-not $p.HasExited) { "OK"; Stop-Process $p.Id -Force }; Get-Content err.txt
```

### MSVC treats some warnings as errors

`CMakeLists.txt` sets `/W4` plus `/we4715` (not all control paths return a value)
and `/we4239` (binding a temporary to a non-const reference). Code that trips these
fails the build, not just warns. (The repo's path name is Chinese — MSVC prints it
as `???`; harmless.)

## Engine architecture (the parts that span files)

**ECS (`src/core/tiny_ecs.hpp`).** Component-centric, no archetypes. Each component
type `T` has a single global container `ECS::registry<T>`. Patterns used everywhere:

- `entity.insert<T>(...)` / `entity.emplace<T>(...)`, `entity.get<T>()`,
  `entity.has<T>()`, `entity.remove<T>()`.
- Systems iterate `ECS::registry<T>.entities` (a `std::vector<Entity>`) and
  `.get<U>()` sibling components. There is no "system" base class — systems are just
  classes with a `step()`.
- `ECS::ContainerInterface::remove_all_components_of(e)` strips an entity from every
  registry. **`WorldSystem::restart()` removes every entity that has a `Motion`** to
  reset a level — components attached only to Motion-bearing entities die on reload.

**Main loop (`src/core/main.cpp`).** Constructs `WorldSystem` (window, input, audio,
level lifecycle), `RenderSystem` (all drawing), `PhysicsSystem` (SAT collision +
impulse resolution). Variable-timestep loop: `world.step` → `physics.step` →
`world.handle_collisions` → `renderer.draw`. `GameInstance` (static) holds global
frame/game time and speed multipliers consumed across systems.

**Rendering (`src/rendering/`).** Immediate-style, not retained. `RenderSystem::draw`
painter-sorts `ShadedMeshRef` entities by `Motion.zValue` and draws each with an
axis-aligned orthographic projection. Resources are deduped by string key via
`cache_resource(key)` returning a `ShadedMesh` (mesh + `Effect` shader program +
`Texture`); `GLResource<>` (in `render_components.hpp`) is a move-only RAII wrapper
around GL handles. Forward shaders are loaded from
`data/shaders/<name>.{vertex,fragment}.glsl` (`#version 330`).

> `Camera` (`rendering/Camera.hpp`) already has oblique world↔screen helpers
> (`oblique_x_scale`/`oblique_y_scale`), but the **forward** path does not use them —
> only the deferred lighting pipeline applies a true oblique projection.

**Levels (`src/gameplay/systems/levelLoader.cpp`).** Data-driven: JSON files in
`data/levels/<name>.json`. `LevelLoader::level_objects` maps a JSON key (e.g.
`"blocks"`, `"movable_wall"`) to a spawner lambda; register new entity types there.
`template.json` is intentionally empty.

**Include style.** `CMakeLists.txt` adds each `src/` subdirectory to the include
path, so headers are included by bare filename (`#include "render.hpp"`), with the
lighting module reached as `"lighting/Foo.hpp"` from `src/rendering`.

## Deferred lighting system (`src/rendering/lighting/` + `data/shaders/lighting/`)

The major architectural addition; full detail in [docs/LIGHTING.md](docs/LIGHTING.md).
Things to know before editing it:

- **Requires OpenGL 4.3 core** (compute shaders + image load/store). The context
  hint is set in `world.cpp` (`GLFW_CONTEXT_VERSION_MAJOR/MINOR = 4/3`). The legacy
  `#version 330` forward shaders still run under it.
- **Integration point:** `DeferredLighting::render` is called inside
  `RenderSystem::draw`, gated by `LightingDemo::active`, *before* the forward
  overlay (UI/particles) passes. It composites the lit world to the back buffer;
  forward draws on top.
- **Driven by ECS:** entities with both `Motion` + `LitSprite` are rendered into the
  G-buffer; `Light` components (point/spot, world space) drive the direct pass.
  `LightingComponents.hpp` defines these plus `ObliqueProjection`.
- **The oblique projection math is duplicated** between C++ (`ObliqueProjection` in
  `LightingComponents.hpp`) and GLSL (`projectWorld`/`unproject` in
  `direct_light.fragment.glsl`, anchor placement in `gbuffer.vertex.glsl`). If you
  change `kx/ky/kz` or the projection formula, **change every copy** or CPU placement
  and GPU unprojection will disagree.
- **Coordinate convention:** screen origin bottom-left, y-up (matches `gl_FragCoord`).
  GLFW mouse coords are top-left, so they are y-flipped in `LightingDemo::update`.
- **The demo scene** is spawned from `main()` via `LightingDemo::setup`, and
  re-spawned on level reload via `LightingDemo::respawn` (called at the end of
  `WorldSystem::restart`, because the entity wipe deletes the lit sprites). Controls:
  mouse aims the flashlight; number keys `0`–`8` pick G-buffer/SDF/GI/light debug
  views; `B` toggles the Radiance Cascades bilinear fix.
- `GlLightingUtil` is the only place that loads **compute** programs and creates
  GL render targets; the engine's `Effect` only does vertex+fragment.
