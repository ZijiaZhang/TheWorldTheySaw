# Pseudo-3D Oblique Deferred Lighting

An implementation of the design doc *「伪3D 斜视角 Sprite 游戏 — 动态光照系统」* on top of
the engine: a fully 2D sprite pipeline that reconstructs a real 3D point + world
normal per pixel and runs genuine 3D lighting math, plus 2D Radiance Cascades for
global illumination.

> Everything is 2D — no 3D meshes are ever built. The third dimension comes from a
> per-pixel **height** channel and a per-sprite **surface orientation**, exactly as
> in §3 of the design.

## Requirements

OpenGL **4.3 core** (compute shaders + image load/store). The GLFW context hint was
raised from 3.3 to 4.3 in `world.cpp`; the legacy `#version 330` forward shaders
still run unchanged under it.

## The pipeline (one frame)

| Pass | What | Files |
|---|---|---|
| 1. Geometry | Sprites → 4-MRT G-buffer; tangent normals baked to **world space** via per-sprite TBN | `gbuffer.{vertex,fragment}.glsl`, `DeferredLighting::geometryPass` |
| 2. SDF (JFA) | Occluder mask → signed distance field via Jump Flooding (compute) | `sdf_{seed,jfa,distance}.compute.glsl`, `SdfPass` |
| 3. Radiance Cascades | Noise-free ambient / bounced / emissive GI (compute) | `rc_{cascade,merge,resolve}.compute.glsl`, `RadianceCascades` |
| 4+5. Direct light | Deferred spotlight/point lights in world space + height-field ray-march shadows + constant-view specular | `direct_light.fragment.glsl` |
| 6. Composite | `albedo·(gi·ao + ambient) + direct + emissive`, ACES tonemap | `composite.fragment.glsl` |

### G-buffer layout (§5)

| Attach | Format | Contents |
|---|---|---|
| 0 | RGBA8 | albedo · **occluder mask** (a) |
| 1 | RGBA8 | world normal (encoded) · roughness (a) |
| 2 | R16F | height / elevation |
| 3 | RGBA16F | emissive (rgb) · **coverage** (a) |

Emissive's alpha doubles as a 1-bit coverage flag so the direct/composite passes
skip background pixels.

### Oblique projection (§2)

`ObliqueProjection` (in `LightingComponents.hpp`) maps world `(wx,wy,wz)` ↔ screen
pixels (origin bottom-left, y up) and is duplicated verbatim in the GLSL so CPU
placement and GPU unprojection agree exactly:

```
sx = cx + (ex - ey)·kx
sy = cy + (ex + ey)·ky + wz·kz      ex = wx - focus.x, ey = wy - focus.y
```

Each lit pixel is unprojected from `gl_FragCoord + height`, giving a true 3D `P`;
attenuation uses the **3D** distance, which is what makes a "light on the floor" and
a "light on a wall" read completely differently.

### Surface orientation (§3.2 — the core trick)

A flat sprite carries a `SurfaceType`:
* **Floor** → world normal +Z (TBN = identity).
* **Wall** → world normal horizontal/outward (`facingAngle`, 0 = faces −Y).

The geometry pass rotates the tangent-space normal-map sample into world space with
that TBN, so the *same* normal map lights correctly whether it's a floor or a wall.

### Radiance Cascades layout

Base spacing `s0 = 2`, base directions `2×2 = 4`. Every cascade texture is the same
size (GI resolution); cascade `c` partitions it into probes spaced `s_c = s0·2^c`
apart, each owning an `s_c × s_c` block = `4^(c+1)` directions. Merge runs top-down
with a **bilinear probe weight** (Bilinear Fix, Osborne & Sannikov 2024) to kill ring
artifacts; `resolve` integrates C0 over direction into a per-pixel irradiance texture.

## Demo controls

The demo scene (floor, brick walls, rounded props, two emissive lamps, a mouse-aimed
flashlight) is spawned from `main()` via `LightingDemo::setup`.

* **Move the mouse** — aim the flashlight (a shadow-casting spotlight).
* **Number keys 0–8** — debug views:
  `0` final · `1` albedo · `2` world normal · `3` height · `4` SDF ·
  `5` GI (Radiance Cascades) · `6` direct light · `7` emissive · `8` occluder mask.
* **B** — toggle the RC bilinear fix (watch the ring artifacts appear/disappear).

## Tuning

Everything visual is data, not code:

* Projection: `LightingDemo::projection` (`tileW`, `tileH`, `heightScale`).
* Look knobs: public fields on `DeferredLighting` (`giScale`, `viewDir`, `shininess`,
  `specStrength`, `ambient`, `background`, `aoRadius`, `shadow*`).
* GI: `RadianceCascades::{baseInterval, gain, bilinearFix}`.
* Per-sprite material: the `LitSprite` component.

GI runs at half resolution by default (`giScale = 0.5`, §15) and is upsampled at
composite. Only lights with `castsShadow` pay for a height-field march, so keep the
count of shadowing lights small and let the cascades carry the ambient soft light —
the "few sharp main lights + lots of soft emissive" split from §15.

## Notes / where to look first if something looks off

* The direct lighting, world-space normal mapping and height shadows (milestones
  M2–M5) are the highest-confidence parts and give the biggest visual payoff.
* Radiance Cascades is the research-grade part; if GI looks wrong, inspect debug
  views `4` (SDF) then `5` (GI), and try `B` / `RadianceCascades::baseInterval`.
