# LIGHTING_DESIGN_CONTRACT.md — Authoritative Deferred Lighting Integration Contract

> This is the single source of truth for the dynamic deferred-lighting pipeline added to the
> `灯下黑` C++/OpenGL ECS template. Every shader file and every C++ module MUST conform to this
> document **literally**. Where this contract and the design doc
> (`动态光照系统设计文件.md`) disagree on a platform detail, **this contract wins** (the design
> doc assumes GL 4.3 + compute; we implement the doc's explicitly-permitted multi-pass
> fragment-shader form on GL 3.3 core). Where they disagree on *intent*, the design doc's two
> **iron rules** are inviolable:
>
> 1. **The flashlight is a deferred "hero" light and is NOT fed into Radiance Cascades.** RC
>    produces only ambient/GI/soft emissive light. The flashlight is computed in
>    `direct_light.fragment.glsl`.
> 2. **The HEIGHT channel is the foundation of all pseudo-3D shadows and attenuation.** All 3D-feel
>    shadows and correct falloff derive from `GBuffer2` (height), in the unified "screen + height"
>    space `P = (fragCoord.xy in PIXELS, height in WORLD UNITS)`.

---

## 0. Global conventions (apply everywhere)

- **GL profile:** OpenGL **3.3 core**. No compute shaders, no `imageLoad/Store`, no SSBOs. Every
  lighting pass after the geometry pass is a **full-screen FRAGMENT shader** rendered into an FBO.
- **GLSL version line:** every shader begins with exactly `#version 330` (no `core` suffix, matching
  the existing template shaders).
- **Full-screen passes** are drawn as **one large triangle** generated from `gl_VertexID` with **NO
  vertex buffer and NO VAO attributes bound** (a dummy/empty VAO must still be bound because core
  profile forbids drawing with VAO 0). They share `fullscreen.vertex.glsl`. Draw call:
  `glDrawArrays(GL_TRIANGLES, 0, 3)`.
- **Working space `P`:** `P.xy = gl_FragCoord.xy` in **pixels** (origin bottom-left, GL convention);
  `P.z = height` in **world units** sampled from `GBuffer2`. Lights are expressed in this same
  space: a light position is `vec3(px, px, height_world_units)`.
- **uv convention:** `uv = gl_FragCoord.xy / uResolution` for full-screen passes, where
  `uResolution` is the render-target size in pixels (`vec2`). `vUV` from the vertex shader equals
  this same value at fragment centers. **uv origin is bottom-left** (consistent with `gl_FragCoord`
  and with the existing sprite texcoords where `texcoord=(0,0)` is the bottom-left vertex).
- **Float formats used (all color-renderable in 3.3 core):** `R16F`, `RG16F`, `RGBA16F`, plus
  `RGBA8`. Depth uses `GL_DEPTH24_STENCIL8` (`D24S8`) as a renderbuffer (optional, sort/effects
  only — lighting never reads depth).
- **No premultiplied alpha anywhere.** Albedo is straight (non-premultiplied).
- **Color space:** all textures are treated as **linear** for lighting math (the template does not
  use sRGB textures; demo assets are authored linear). Tonemapping happens once, in
  `composite.fragment.glsl`.
- **Sampler filtering:** GI/RC/composite read with `GL_LINEAR`. G-buffer normal/height/occluder and
  the SDF/JFA seed textures read with `GL_NEAREST` (linear filtering of encoded normals, heights,
  and seed coordinates produces wrong interpolated values at edges). Wrap mode is
  `GL_CLAMP_TO_EDGE` on every lighting texture.
- **Clear values:** see each FBO below. Where "invalid seed" is needed, the sentinel is the literal
  `vec2(-1.0, -1.0)` written into an `RG16F` target.

---

## 1. G-buffer attachments & exact formats (design §3)

The G-buffer is one FBO with **4 color attachments** (MRT) + a depth-stencil renderbuffer. All four
color textures are the **full render resolution** `W×H` (the framebuffer pixel size).

| Attachment            | GL internal format | Channels / meaning                                                                 | Clear value            |
|-----------------------|--------------------|------------------------------------------------------------------------------------|------------------------|
| `GBuffer0` (loc 0)    | `GL_RGBA8`         | `rgb = albedo` (linear), `a = occluderMask` (1.0 = opaque blocker, 0.0 = not)       | `(0,0,0,0)`            |
| `GBuffer1` (loc 1)    | `GL_RGBA8`         | `rg = encode(normal.xy)` = `normal.xy*0.5+0.5`; `b = roughness` (or matID); `a` spare | `(0.5,0.5,1.0,0.0)`    |
| `GBuffer2` (loc 2)    | `GL_R16F`          | `r = height` (world units), in space `P.z`                                          | `0.0`                  |
| `GBuffer3` (loc 3)    | `GL_RGBA16F`       | `rgb = emissive` (linear radiance), `a` spare                                       | `(0,0,0,0)`            |
| depth-stencil         | `GL_DEPTH24_STENCIL8` renderbuffer | optional sort/effects; **not read by lighting**                    | `1.0 / 0`              |

`glDrawBuffers` MUST be called with exactly
`{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3}` in that
order so `layout(location=N)` matches.

### 1.1 Normal encode / decode convention (NORMATIVE)

Normals live in **"screen + height" space**, NOT world space and NOT raw tangent space. The axes of
this space are:

- `+x` = screen-right (increasing pixel x),
- `+y` = screen-up (increasing pixel y / `gl_FragCoord.y`),
- `+z` = "out of the floor, toward higher elevation" (the height axis, same axis as `P.z`).

A flat **ground** sprite has normal `(0, 0, 1)` (points straight up the height axis). A **wall**
sprite that faces the camera-and-slightly-up has a normal with positive `z` and a positive `y`
component (it "leans toward the viewer"). The geometry pass is responsible for delivering normals
already expressed in this basis (see §4.1: the sprite's tangent-space normal map is reoriented by
the per-sprite `uNormalToSurface` mat3, default identity for ground-aligned art).

**Encode (geometry fragment shader):**
```glsl
// n is the unit normal in screen+height space (z >= 0 for visible surfaces).
// Y is negated because the world->screen projection (render.cpp: sy = 2/(top-bottom) < 0)
// reflects local +Y to screen-DOWN, while the lighting passes work in gl_FragCoord
// (Y up). Without this flip a +Y-up tangent-space normal map is mirrored and the lit
// region orbits opposite to the light. n.x needs no flip (sx > 0); flat normals
// (n.y = 0) and the nz reconstruction (depends on n.y^2) are sign-invariant. The flip
// is applied AFTER uNormalToSurface so per-sprite rotation still composes.
oNormalMat.rg = vec2(n.x, -n.y) * 0.5 + 0.5;   // store xy in [0,1]
```

**Decode (every lighting pass that needs the normal):**
```glsl
vec2 nxy = texture(uGNormal, uv).rg * 2.0 - 1.0;   // back to [-1,1]
float nz = sqrt(max(0.0, 1.0 - dot(nxy, nxy)));    // reconstruct z, assumed >= 0
vec3 N = vec3(nxy, nz);                             // already unit-length
```
The reconstructed `z` is always taken **non-negative**; surfaces never face "into" the floor.

### 1.2 Height write convention (NORMATIVE)

```glsl
// uBaseHeight: constant base elevation of this sprite (world units)
// uHeightRange: how much the heightmap's [0,1] red channel adds on top (world units)
// uHasHeightMap: 1.0 if a per-texel heightmap is bound, else 0.0
float hmap = (uHasHeightMap > 0.5) ? texture(uHeightMap, vUV).r : 0.0;
oHeight = uBaseHeight + hmap * uHeightRange;
```
For a flat-height sprite, bind no heightmap (`uHasHeightMap = 0`) and `oHeight = uBaseHeight`.

---

## 2. Intermediate FBO textures & formats

All intermediate targets are created once at init and resized with the window. Unless stated
"half-res", a target is full render resolution `W×H`. Half-res targets are `ceil(W/2)×ceil(H/2)` and
are documented as such. **Filtering/wrap per §0.**

| Logical name        | GL format   | Resolution | Purpose                                                                                  | Clear        |
|---------------------|-------------|-----------|------------------------------------------------------------------------------------------|--------------|
| `gbuffer0..3`       | see §1      | W×H       | G-buffer MRT                                                                              | see §1       |
| `seedA`, `seedB`    | `RG16F`     | W×H       | JFA ping-pong pair. Each texel holds the **pixel-space xy of the nearest occluder seed** found so far; invalid = `(-1,-1)`. | `(-1,-1)`    |
| `sdfDist`           | `R16F`      | W×H       | Signed distance field in **pixels**: distance to nearest occluder. Positive outside, negative inside occluders. | `0.0`        |
| `cascadeA`,`cascadeB` | `RGBA16F` | see §2.1  | Radiance-cascade ping-pong pair (one cascade's radiance per draw; merged top-down). `rgb = radiance`, `a = transmittance/hit-flag` (see §6.2). | `(0,0,0,0)`  |
| `giResult`          | `RGBA16F`   | W/2×H/2 (half-res, see §7) | Final per-pixel incoming fluence from cascade 0 (the GI term). `rgb = irradiance`, `a` unused. | `(0,0,0,0)`  |
| `directResult`      | `RGBA16F`   | W×H       | Flashlight (+ any future hero lights) direct contribution, already multiplied by shadow. `rgb = light`, `a` unused. | `(0,0,0,0)`  |
| (final composite)   | default FBO | W×H       | Tonemapped output to the back buffer (FBO 0). Not a texture.                              | bg color     |

**Notes**
- `seedA/seedB` and `cascadeA/cascadeB` are ping-pong pairs: each step reads one and writes the
  other; the C++ `PingPong` helper (§ contract part 6) tracks which is "current".
- The SDF is stored as a **single distance** (`R16F`, `sdfDist`) derived from the final JFA seed
  texture in `sdf_distance.fragment.glsl`. The nearest-seed coordinates themselves live in the
  `RG16F` JFA target. Sphere-tracing in RC reads `sdfDist`.
- **GI runs at half resolution** (`giResult` is `W/2×H/2`) and is upsampled with bilinear filtering
  in the composite pass. This is the conservative default (design §12).

### 2.1 Radiance-cascade texture layout (NORMATIVE, simple-but-correct)

We use the **"flat 2D, fixed full-res target, probes stored as tiles"** layout, the simplest layout
that is correct under the Osborne–Sannikov bilinear-fix merge. Both `cascadeA` and `cascadeB` are a
**single `RGBA16F` texture of size `W×H`** (full render res). The *meaning* of the texels changes per
cascade via uniforms; the physical texture size never changes (this is what lets us ping-pong merge
without reallocating).

For cascade `c`:
- Probe grid spacing on screen: `probeSpacing(c) = uBaseProbeSpacing * 2^c` pixels. (`uBaseProbeSpacing`
  default = 2 px, see §7.) So probe count per axis = `ceil(resolution / probeSpacing(c))`.
- Directions per probe: `dirCount(c) = uBaseDirCount * 4^c` (base = 4 → 4, 16, 64, 256, …).
- **Tile packing:** the `W×H` texture is divided into a `D×D` grid of **angular tiles**, where
  `D = 2^c` (so `D*D = 4^c` tiles, one per direction *group*). Tile `(tx,ty)` (with
  `0 ≤ tx,ty < D`) stores, for **every probe**, the radiance for the direction indexed by
  `dirIndex = ty*D + tx`. Within a tile, the probe at grid cell `(px,py)` is written to texel
  `( tx*tileW + px , ty*tileH + py )` where `tileW = floor(W / D)`, `tileH = floor(H / D)`.
  - Cascade 0 (`D=1`) is a single tile = the whole texture; each texel is one probe with its first
    of 4 directions resolved by sub-sampling inside the fragment (see §6.2).
  - This keeps every cascade in the same `W×H` footprint: higher cascades pack more directions
    (more tiles) but fewer probes per tile (coarser spacing), so total texels are conserved — the RC
    invariant from design §6.1.

> Implementors: the exact `dirIndex → direction vector` mapping is fixed in
> `rc_cascade.fragment.glsl` as `angle = 2π * (dirIndex + 0.5) / dirCount(c)`,
> `dir = vec2(cos angle, sin angle)`. Keep this identical in `rc_merge.fragment.glsl`.

---

## 3. Texture-unit assignment map (NORMATIVE — identical across ALL passes)

Every pass binds its inputs to these fixed `GL_TEXTUREn` units, and every sampler uniform is set to
the matching unit index with `glUniform1i`. A pass only binds the units it actually reads; the unit
*number* for a given logical texture is **always** the one below, in every shader, with no
exceptions.

| Unit         | Sampler uniform name | Logical texture        | Read by passes                         |
|--------------|----------------------|------------------------|----------------------------------------|
| `GL_TEXTURE0`| `uGAlbedo`           | `gbuffer0` (albedo+mask) | geometry inputs use 0–3 for *source art*; lighting passes: SDF seed/composite | 
| `GL_TEXTURE1`| `uGNormal`           | `gbuffer1` (normal+rough)| direct, (composite debug)              |
| `GL_TEXTURE2`| `uGHeight`           | `gbuffer2` (height)      | jfa_seed, sdf, direct (shadow)         |
| `GL_TEXTURE3`| `uGEmissive`         | `gbuffer3` (emissive)    | rc_cascade, composite                  |
| `GL_TEXTURE4`| `uSeedTex`           | `seedA`/`seedB` (JFA)    | jfa_step, sdf_distance                 |
| `GL_TEXTURE5`| `uSDF`               | `sdfDist`                | rc_cascade                             |
| `GL_TEXTURE6`| `uUpperCascade`      | `cascadeA`/`cascadeB`    | rc_merge (the N+1 cascade being merged down) |
| `GL_TEXTURE7`| `uGI`                | `giResult`               | composite                              |
| `GL_TEXTURE8`| `uDirect`            | `directResult`           | composite                              |

**Geometry pass (Pass 1) is the exception** — it samples *per-sprite source art*, not the G-buffer.
Its source textures use a separate, self-consistent block (still fixed):

| Unit         | Sampler uniform name | Source art        |
|--------------|----------------------|-------------------|
| `GL_TEXTURE0`| `uAlbedo`            | LitSprite albedo  |
| `GL_TEXTURE1`| `uNormalMap`         | LitSprite normal  |
| `GL_TEXTURE2`| `uHeightMap`         | LitSprite height  |
| `GL_TEXTURE3`| `uEmissiveMap`       | LitSprite emissive|

(The numeric overlap is intentional and harmless: geometry source art and the G-buffer outputs never
coexist in one program. Lighting passes never bind source art.)

`uGAlbedo` is bound on unit 0 wherever a lighting pass needs the albedo (only the composite reads it;
JFA seed reads height on unit 2 and occluder mask via `gbuffer0` on unit 0). See per-pass §5.

---

## 4. Shader files — exact interfaces

Every program is created with `Effect::load_from_file(vs, fs)`. Full-screen passes pass
`shader_path("fullscreen") + ".vertex.glsl"` as `vs` and `shader_path("<pass>") + ".fragment.glsl"`
as `fs`. The geometry pass uses its own `gbuffer.vertex.glsl`.

All listed `uniform` names are **exact**. C++ sets them by `glGetUniformLocation` with these strings.
Samplers are `sampler2D` set via `glUniform1i` to the §3 unit. Vectors/scalars use the obvious
`glUniform*`. **Do not rename or add uniforms** without updating this contract.

---

### 4.1 `fullscreen.vertex.glsl` (shared by ALL full-screen passes)

No vertex attributes. Emits a single clip-space triangle covering the screen and a `[0,1]` uv.

```glsl
#version 330
out vec2 vUV;
void main() {
    // Big-triangle trick: 3 verts, covers the screen, no VBO.
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2); // (0,0),(2,0),(0,2)
    vUV = p;                       // [0,2] -> covers [0,1] over the screen
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
```
- **out** `vec2 vUV` — `[0,1]` across the visible screen, **origin bottom-left**.
- **uniforms:** none.

---

### 4.2 `gbuffer.vertex.glsl` (geometry pass vertex)

Matches the existing sprite vertex convention exactly (`in_position`, `in_texcoord`, mat3
`transform`/`projection`), so the geometry pass can reuse `createSprite`-style meshes.

```glsl
#version 330
in vec3 in_position;
in vec2 in_texcoord;
out vec2 vUV;
uniform mat3 transform;     // per-sprite model transform (Transform::mat), same as forward path
uniform mat3 projection;    // 2D screen projection (projection_2D from RenderSystem::draw)
void main() {
    vUV = in_texcoord;
    vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}
```
- **in** `vec3 in_position`, `vec2 in_texcoord` (attribute names MUST match the template so the
  existing attribute-binding code in `drawTexturedMesh` works).
- **out** `vec2 vUV`.
- **uniforms:** `mat3 transform`, `mat3 projection`.

---

### 4.3 `gbuffer.fragment.glsl` (geometry pass fragment — 4 MRT outputs)

```glsl
#version 330
in vec2 vUV;
layout(location = 0) out vec4 oAlbedoMask; // rgb albedo, a occluder mask
layout(location = 1) out vec4 oNormalMat;  // rg encoded normal.xy, b roughness, a spare
layout(location = 2) out float oHeight;    // height (world units)  -> R16F target
layout(location = 3) out vec4 oEmissive;   // rgb emissive, a spare

uniform sampler2D uAlbedo;       // unit 0
uniform sampler2D uNormalMap;    // unit 1
uniform sampler2D uHeightMap;    // unit 2
uniform sampler2D uEmissiveMap;  // unit 3

uniform float uIsOccluder;       // 1.0 -> writes 1 into albedo.a (blocks light), else 0.0
uniform float uRoughness;        // [0,1] stored into normal.b (or matID)
uniform float uBaseHeight;       // world units
uniform float uHeightRange;      // world units multiplied by heightmap.r
uniform float uHasHeightMap;     // 1.0 if uHeightMap bound, else 0.0
uniform float uHasEmissive;      // 1.0 if uEmissiveMap bound, else 0.0
uniform mat3  uNormalToSurface;  // reorients tangent-space normal into screen+height space
                                 // (identity for ground-aligned art; a tilt matrix for walls)
```
Semantics (NORMATIVE):
- `alb = texture(uAlbedo, vUV); if (alb.a < 0.5) discard;` (alpha clip, matches design §4).
- `vec3 nt = texture(uNormalMap, vUV).xyz * 2.0 - 1.0; vec3 n = normalize(uNormalToSurface * nt);`
  then encode per §1.1. If no normal map is desired, the integrator binds a flat normal texture
  `(0.5,0.5,1.0)` so `nt = (0,0,1)`.
- `oAlbedoMask = vec4(alb.rgb, uIsOccluder);`
- `oNormalMat  = vec4(vec2(n.x, -n.y) * 0.5 + 0.5, uRoughness, 0.0);` (Y flip per §1.1).
- `oHeight`, see §1.2.
- `oEmissive  = (uHasEmissive > 0.5) ? texture(uEmissiveMap, vUV) : vec4(0.0);`

Blending in the geometry pass is **disabled** (`glDisable(GL_BLEND)`); transparency is handled by
the alpha-clip `discard`, not blending, so MRT writes stay consistent.

---

### 4.4 `jfa_seed.fragment.glsl` (JFA initialization)

Writes the seed coordinate for occluder texels, sentinel otherwise.
```glsl
#version 330
in vec2 vUV;
layout(location = 0) out vec2 oSeed;     // RG16F: nearest-seed xy in PIXELS, or (-1,-1)
uniform sampler2D uGAlbedo;              // unit 0 (occluder mask in .a)
uniform vec2 uResolution;                // render-target size in pixels
void main() {
    float occ = texture(uGAlbedo, vUV).a;
    oSeed = (occ > 0.5) ? (gl_FragCoord.xy) : vec2(-1.0, -1.0);
}
```
- **out** `vec2 oSeed`.
- **uniforms:** `sampler2D uGAlbedo` (unit 0), `vec2 uResolution`.

---

### 4.5 `jfa_step.fragment.glsl` (one JFA jump)

One flood step at a given offset; run `ceil(log2(maxdim))` times with halving offset.
```glsl
#version 330
in vec2 vUV;
layout(location = 0) out vec2 oSeed;     // RG16F
uniform sampler2D uSeedTex;              // unit 4 (previous ping-pong seed texture)
uniform vec2  uResolution;               // pixels
uniform float uOffset;                   // jump step in PIXELS for this round (N/2,N/4,...,1)
void main() {
    vec2 best = texture(uSeedTex, vUV).xy;
    float bestD = (best.x < 0.0) ? 1e20 : distance(best, gl_FragCoord.xy);
    for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++) {
        vec2 sampUV = (gl_FragCoord.xy + vec2(dx, dy) * uOffset) / uResolution;
        vec2 cand = texture(uSeedTex, sampUV).xy;
        if (cand.x < 0.0) continue;
        float d = distance(cand, gl_FragCoord.xy);
        if (d < bestD) { bestD = d; best = cand; }
    }
    oSeed = best;
}
```
- **out** `vec2 oSeed`.
- **uniforms:** `sampler2D uSeedTex` (unit 4), `vec2 uResolution`, `float uOffset` (pixels).

---

### 4.6 `sdf_distance.fragment.glsl` (seed → signed distance)

Converts the final JFA seed texture into a signed pixel distance.
```glsl
#version 330
in vec2 vUV;
layout(location = 0) out float oDist;    // R16F: signed distance in PIXELS (+outside, -inside)
uniform sampler2D uSeedTex;              // unit 4 (final JFA seeds)
uniform sampler2D uGAlbedo;              // unit 0 (occluder mask to sign the distance)
uniform vec2 uResolution;                // pixels
void main() {
    vec2 seed = texture(uSeedTex, vUV).xy;
    float d = (seed.x < 0.0) ? 1e20 : distance(seed, gl_FragCoord.xy);
    float inside = (texture(uGAlbedo, vUV).a > 0.5) ? -1.0 : 1.0;
    oDist = d * inside;
}
```
- **out** `float oDist`.
- **uniforms:** `sampler2D uSeedTex` (unit 4), `sampler2D uGAlbedo` (unit 0), `vec2 uResolution`.

---

### 4.7 `rc_cascade.fragment.glsl` (one cascade's ray cast over SDF + emissive)

Casts, for the probe(s) packed into this texel's tile, the directions of cascade `uCascadeIndex`
along their `[tNear,tFar]` interval, sphere-tracing `uSDF` and gathering `uGEmissive`.
```glsl
#version 330
in vec2 vUV;
layout(location = 0) out vec4 oRadiance;   // RGBA16F: rgb radiance, a = 1.0 if ray hit, 0.0 miss
uniform sampler2D uSDF;                     // unit 5 (signed distance, pixels)
uniform sampler2D uGEmissive;              // unit 3 (emissive radiance)
uniform vec2  uResolution;                 // render-target pixels (W,H)
uniform int   uCascadeIndex;               // c = 0,1,2,...
uniform float uBaseProbeSpacing;           // pixels (cascade 0 spacing); spacing = base * 2^c
uniform int   uBaseDirCount;               // 4 -> dirCount = base * 4^c
uniform float uD0;                         // cascade-0 ray length (pixels); interval length = uD0*4^c
uniform float uIntervalNear;              // tNear for this cascade (pixels) = sum of lower intervals
uniform int   uMaxSteps;                   // sphere-trace step cap per ray
uniform float uEps;                        // hit threshold (pixels), e.g. 1.0
uniform float uMinStep;                    // min sphere-trace advance (pixels), e.g. 0.5
```
Semantics (NORMATIVE):
- `D = 1 << uCascadeIndex;` tile coords `tx,ty` from `floor(gl_FragCoord.xy / vec2(tileW,tileH))`
  where `tileW=floor(W/D)`, `tileH=floor(H/D)`; `dirIndex = ty*D + tx`; the per-probe sub-pixel
  inside the tile gives `probePixel = (probeCellIndex + 0.5) * probeSpacing`.
- `dirCount = uBaseDirCount << (2*uCascadeIndex);`
  `angle = 6.2831853 * (float(dirIndex) + 0.5) / float(dirCount);`
  `dir = vec2(cos(angle), sin(angle));`
- `tNear = uIntervalNear; tFar = uIntervalNear + uD0 * float(1 << (2*uCascadeIndex));`
- Sphere-trace from `probePixel + dir*tNear` to `tFar`: advance `t += max(d, uMinStep)`; on
  `d < uEps` sample `uGEmissive` at that pixel → `oRadiance = vec4(emissive, 1.0)` (hit). If the
  interval ends with no hit → `oRadiance = vec4(0.0, 0.0, 0.0, 0.0)` (miss; `a=0` tells the merge
  pass to pull from the upper cascade).
- **uniforms:** as listed above (all sampler units per §3).

---

### 4.8 `rc_merge.fragment.glsl` (bilinear-fix merge of cascade N with N+1)

Merges the just-cast cascade `N` (input bound as the radiance currently in the ping-pong "current"
slot, conceptually the same texel layout) with the already-merged cascade `N+1` (`uUpperCascade`),
using the **Osborne & Sannikov 2024 bilinear fix**: when a ray of cascade N misses, fetch the four
spatially-neighboring probes of cascade N+1 for the *matching* direction group and bilinearly weight
them by the probe's sub-cell position, instead of nearest-probe lookup (this removes ring artifacts).
```glsl
#version 330
in vec2 vUV;
layout(location = 0) out vec4 oMerged;     // RGBA16F merged radiance for cascade N
uniform sampler2D uUpperCascade;           // unit 6 (already-merged cascade N+1)
uniform sampler2D uSDF;                     // unit 5 (for recomputing this cascade's near-interval)
uniform sampler2D uGEmissive;              // unit 3
uniform vec2  uResolution;                 // pixels
uniform int   uCascadeIndex;               // N (the cascade being produced)
uniform float uBaseProbeSpacing;
uniform int   uBaseDirCount;
uniform float uD0;
uniform float uIntervalNear;
uniform int   uMaxSteps;
uniform float uEps;
uniform float uMinStep;
```
Semantics (NORMATIVE):
- Recompute the near-interval radiance for cascade N exactly as in `rc_cascade.fragment.glsl`
  (same uniforms, same `dirIndex`/`angle`/`tNear`/`tFar`). Call it `near.rgb`, hit flag `near.a`.
- If `near.a >= 0.5` (hit in the near interval): `oMerged = vec4(near.rgb, 1.0)` — occluder/emitter
  found locally, do not pull from above.
- Else (miss): each cascade-N direction maps to **4 directions** of cascade N+1 (the 1→4 angular
  refinement). Average those 4 upper directions, **bilinearly weighted across the 4 nearest N+1
  probes** by this probe's fractional position in the N+1 probe grid. `oMerged.rgb = bilinearFix(...)`,
  `oMerged.a = 1.0`.
- The 1→4 direction mapping: cascade N direction `dirIndex` corresponds to cascade N+1 directions
  `{4*dirIndex + k | k=0..3}` under the `angle = 2π*(idx+0.5)/dirCount` convention. Keep this exact.
- Cascade 0's merge result, sampled per screen pixel, is the GI fluence written to `giResult` by the
  final (N=0) invocation (the C++ binds `giResult` as the render target for the last merge; see §5).
- **uniforms:** as listed (same set as 4.7 plus `uUpperCascade` on unit 6).

> The merge runs **top-down**: produce highest cascade first (no upper → it merges against a black
> `uUpperCascade`), then N-1, …, down to 0. The final N=0 pass renders into `giResult` (half-res).

---

### 4.9 `direct_light.fragment.glsl` (deferred flashlight spotlight + height-field shadow)

The hero light. Uses decoded normal + height. Implements the spotlight math of design §7 and the
height-field ray-march shadow of design §8. Writes the direct term (already × shadow) to
`directResult`.
```glsl
#version 330
in vec2 vUV;
layout(location = 0) out vec4 oDirect;     // RGBA16F: rgb = flashlight contribution, a = 1.0
uniform sampler2D uGNormal;                // unit 1 (encoded normal + roughness)
uniform sampler2D uGHeight;                // unit 2 (height field; also used for shadow march)
uniform vec2  uResolution;                 // pixels

// --- spotlight params, all in "screen+height" space ---
uniform vec3  uLightPos;                   // (px, px, height_world_units)
uniform vec2  uSpotDir;                     // spotlight aim direction in screen plane (normalized)
uniform float uCosInner;                   // cos(inner cone half-angle); full intensity inside
uniform float uCosOuter;                   // cos(outer cone half-angle); zero beyond
uniform vec3  uLightColor;                 // linear RGB radiant intensity
uniform float uK1;                         // linear attenuation coefficient
uniform float uK2;                         // quadratic attenuation coefficient

// --- height-field shadow march params ---
uniform float uShadowStepLen;              // march step in PIXELS along screen toward light
uniform int   uShadowSteps;                // max steps
uniform float uShadowBias;                 // height bias to avoid self-shadow (world units)
uniform float uShadowStartBias;            // initial offset (pixels) before first sample
```
Semantics (NORMATIVE), matching design §7–§8:
```glsl
vec3  N    = decodeNormal(texture(uGNormal, uv).rg);          // §1.1
vec3  P    = vec3(gl_FragCoord.xy, texture(uGHeight, uv).r);   // screen + height
vec3  toL  = uLightPos - P;
float dist = length(toL);
vec3  L    = toL / max(dist, 1e-4);
float ndotl= max(dot(N, L), 0.0);
float theta= dot(-L.xy, normalize(uSpotDir));                 // cone test in screen plane
float cone = smoothstep(uCosOuter, uCosInner, theta);
float atten= 1.0 / (1.0 + uK1*dist + uK2*dist*dist);
float shadow = traceHeightShadow(uv, P, uLightPos);          // returns [0,1]
oDirect = vec4(uLightColor * ndotl * cone * atten * shadow, 1.0);
```
`traceHeightShadow` (design §8): march `t` from `uShadowStartBias` in steps of `uShadowStepLen`
along `normalize(uLightPos.xy - P.xy)`; raise a virtual ray height by `elevTan*stepLen` per step
where `elevTan = (uLightPos.z - P.z) / length(uLightPos.xy - P.xy)`; if a sampled scene height
`texture(uGHeight, sp).r > rayHeight + uShadowBias`, return `0.0` (occluded); if the march exits the
screen or finishes, return `1.0` (lit). `sp = uv + dirToLight * t / uResolution`.

> **Iron rule 1** is enforced here: the flashlight contribution is computed only in this shader and
> is never written into any cascade/emissive target.

---

### 4.10 `composite.fragment.glsl` (final combine + ACES tonemap)

```glsl
#version 330
in vec2 vUV;
layout(location = 0) out vec4 oColor;      // to default framebuffer (FBO 0)
uniform sampler2D uGAlbedo;                // unit 0 (albedo in rgb)
uniform sampler2D uGEmissive;              // unit 3 (emissive in rgb)
uniform sampler2D uGI;                     // unit 7 (RC GI, half-res; sampled bilinear/upsampled)
uniform sampler2D uDirect;                 // unit 8 (flashlight result)
uniform float uExposure;                   // pre-tonemap multiply (default 1.0)
```
Semantics (NORMATIVE, design §9):
```glsl
vec3 albedo   = texture(uGAlbedo,   vUV).rgb;
vec3 gi       = texture(uGI,        vUV).rgb;   // bilinear upsample from half-res
vec3 direct   = texture(uDirect,    vUV).rgb;
vec3 emissive = texture(uGEmissive, vUV).rgb;
vec3 color    = albedo * (gi + direct) + emissive;
color *= uExposure;
oColor = vec4(acesTonemap(color), 1.0);          // Narkowicz ACES fit, then output
```
`acesTonemap` is the standard Narkowicz fit (`(x*(a*x+b))/(x*(c*x+d)+e)`, `a=2.51,b=0.03,c=2.43,
d=0.59,e=0.14`), clamped to `[0,1]`. No sRGB encode (template framebuffer is plain RGBA8; if the
integrator later enables an sRGB default framebuffer, drop the manual encode — none is added here).

---

## 5. Per-frame PASS SEQUENCE

All full-screen passes: bind the dummy VAO, `glDisable(GL_DEPTH_TEST)`, `glDisable(GL_BLEND)`
(direct/GI write, they don't accumulate across passes here), set viewport to the **target's**
resolution, `glDrawArrays(GL_TRIANGLES,0,3)`.

| # | Pass            | Program (vs+fs)                       | Bind FBO → target(s)              | Samples (unit)                                   | Viewport |
|---|-----------------|---------------------------------------|----------------------------------|--------------------------------------------------|----------|
| 1 | Geometry        | `gbuffer.vertex` + `gbuffer.fragment` | G-buffer FBO → gbuffer0..3       | per-sprite art: uAlbedo(0),uNormalMap(1),uHeightMap(2),uEmissiveMap(3) | W×H |
| 2a| JFA seed        | `fullscreen` + `jfa_seed`             | seed FBO → `seedA`               | uGAlbedo(0)                                      | W×H |
| 2b| JFA steps (×log2)| `fullscreen` + `jfa_step`            | ping-pong seedA↔seedB            | uSeedTex(4)                                       | W×H |
| 2c| SDF distance    | `fullscreen` + `sdf_distance`         | sdf FBO → `sdfDist`              | uSeedTex(4, final seed), uGAlbedo(0)             | W×H |
| 3a| RC cast (top)   | `fullscreen` + `rc_cascade`           | cascade FBO → cascade(cur)       | uSDF(5), uGEmissive(3)                            | W×H |
| 3b| RC merge (top→0)| `fullscreen` + `rc_merge`             | ping-pong cascadeA↔cascadeB; **final N=0 → `giResult`** | uUpperCascade(6), uSDF(5), uGEmissive(3) | W×H (N≥1), W/2×H/2 (N=0 into giResult) |
| 4 | Direct light    | `fullscreen` + `direct_light`         | direct FBO → `directResult`      | uGNormal(1), uGHeight(2)                          | W×H |
| 6 | Composite       | `fullscreen` + `composite`            | default FBO 0 → back buffer      | uGAlbedo(0), uGEmissive(3), uGI(7), uDirect(8)   | W×H |

Notes:
- **Pass 4 and Pass 5 are one shader** (`direct_light.fragment.glsl` contains `traceHeightShadow`),
  matching design §7 ("Pass 4 与 5 通常在同一 shader 内完成").
- Pass 3 ordering: cast+merge are interleaved per cascade, produced **highest cascade first** then
  merged downward, exactly as §4.8 describes. For a first integration the simplest correct loop is:
  for `c = numCascades-1 .. 0`: run `rc_cascade` for `c` into a scratch, then `rc_merge` for `c`
  reading the previous (upper) merged result; the `c==0` merge renders into `giResult`.
- GI is half-res: only the final (`c==0`) merge target (`giResult`) is half-res; the higher
  cascades remain full `W×H` in the ping-pong pair. The composite bilinearly upsamples `giResult`.

---

## 6. C++ interface SPEC (signatures only — orchestrator implements bodies)

Header lives at `src/rendering/deferred_lighting.hpp` (new). It depends only on existing template
types (`Texture`, `Effect`, `GLResource<>`, `Motion`, `Camera`, `vec2/vec3/ivec2`, GL3W). All FBO
wrappers own their textures via `GLResource<TEXTURE>` and a `GLResource<...>`-style framebuffer
handle (add `FRAME_BUFFER` to the `GLResourceType` enum, or store raw `GLuint` framebuffers and
delete them in the destructor — integrator's choice, but **must not leak**).

```cpp
#pragma once
#include "common.hpp"
#include "render_components.hpp"
#include "Camera.hpp"

// ---- ECS component consumed by the geometry pass -------------------------------------------
// One LitSprite per drawable. The geometry pass (Pass 1) reads these textures + scalars and
// writes the 4 MRT outputs. baseHeight/heightRange feed GBuffer2; isOccluder feeds GBuffer0.a.
struct LitSprite {
    Texture albedo;        // required; .a alpha-clips
    Texture normal;        // optional; bind flat (0.5,0.5,1) if absent
    Texture height;        // optional; per-texel height in [0,1] -> scaled by heightRange
    Texture emissive;      // optional; linear emissive radiance
    float   baseHeight  = 0.0f;   // world units, uBaseHeight
    float   heightRange = 0.0f;   // world units, uHeightRange (0 => flat)
    float   roughness   = 1.0f;   // [0,1] -> GBuffer1.b
    bool    isOccluder  = false;  // true => writes 1.0 into GBuffer0.a (blocks light)
    bool    hasHeightMap = false; // -> uHasHeightMap
    bool    hasEmissive  = false; // -> uHasEmissive
    mat3    normalToSurface = mat3(1.0f); // -> uNormalToSurface (identity = ground-aligned)
};

// ---- small helpers --------------------------------------------------------------------------
// Draws the shared full-screen triangle. Binds a dummy VAO, glDrawArrays(GL_TRIANGLES,0,3).
// The program + uniforms must already be bound by the caller.
void drawFullScreenTriangle();

// Ping-pong pair of identically-formatted color textures behind two FBOs.
struct PingPong {
    GLResource<TEXTURE> tex[2];
    GLuint fbo[2] = {0, 0};
    int cur = 0;                                  // index of the "current"/read texture
    void create(ivec2 size, GLenum internalFormat, GLenum filter); // GL_NEAREST or GL_LINEAR
    void resize(ivec2 size, GLenum internalFormat, GLenum filter);
    GLuint readTex()  const;                       // tex[cur]
    GLuint writeFbo() const;                        // fbo[cur ^ 1]
    void   swap();                                  // cur ^= 1
    void   destroy();
};

// ---- pass / resource modules ----------------------------------------------------------------
class GBuffer {
public:
    void create(ivec2 size);
    void resize(ivec2 size);
    void bindForWrite();          // bind FBO + glDrawBuffers(4) + set viewport
    GLuint albedoMask() const;    // gbuffer0
    GLuint normalMat()  const;    // gbuffer1
    GLuint height()     const;    // gbuffer2
    GLuint emissive()   const;    // gbuffer3
    ivec2  size() const;
    void destroy();
private:
    GLResource<TEXTURE> g0, g1, g2, g3;
    GLuint fbo = 0; GLuint depthStencilRbo = 0; ivec2 dim{0,0};
};

class SdfPass {
public:
    void create(ivec2 size);
    void resize(ivec2 size);
    // Runs jfa_seed, ceil(log2(maxdim)) jfa_step rounds, then sdf_distance.
    // Reads occluder mask from gbuffer0 (unit 0).
    void generate(const GBuffer& gbuffer);
    GLuint sdfTexture() const;    // sdfDist (unit 5 source)
    void destroy();
private:
    PingPong seeds;               // RG16F, GL_NEAREST
    GLResource<TEXTURE> sdfDist;  // R16F
    GLuint sdfFbo = 0;
    Effect seedFx, stepFx, distFx;
    ivec2 dim{0,0};
};

class RadianceCascades {
public:
    struct Params {
        int   numCascades      = 4;      // §7 default
        float baseProbeSpacing = 2.0f;   // px (cascade 0)
        int   baseDirCount     = 4;      // 4,16,64,...
        float d0               = 16.0f;  // px, cascade-0 ray length
        int   maxSteps         = 32;     // sphere-trace cap per ray
        float eps              = 1.0f;   // hit threshold (px)
        float minStep          = 0.5f;   // min advance (px)
        bool  halfResGI        = true;   // giResult at W/2 x H/2
        bool  bilinearFix      = true;   // Osborne-Sannikov 2024 (always on)
    };
    void create(ivec2 size, const Params& p);
    void resize(ivec2 size);
    // Casts + merges all cascades top-down using sdf + emissive; fills giResult.
    void compute(const SdfPass& sdf, const GBuffer& gbuffer);
    GLuint giResult() const;      // RGBA16F (half-res if halfResGI)
    const Params& params() const;
    void destroy();
private:
    PingPong cascades;            // RGBA16F, GL_LINEAR
    GLResource<TEXTURE> gi;       // RGBA16F (half-res)
    GLuint giFbo = 0;
    Effect castFx, mergeFx;
    Params prm; ivec2 dim{0,0};
};

class DirectLightPass {
public:
    void create(ivec2 size);
    void resize(ivec2 size);
    // Renders flashlight (+ future hero lights) with height-field shadow into directResult.
    void render(const GBuffer& gbuffer, const struct Spotlight& flashlight);
    GLuint result() const;        // directResult, RGBA16F (unit 8 source)
    void destroy();
private:
    GLResource<TEXTURE> directResult; // RGBA16F
    GLuint fbo = 0;
    Effect fx;                        // fullscreen + direct_light
    ivec2 dim{0,0};
};

class CompositePass {
public:
    void create();                // no owned target (renders to FBO 0)
    // Combines albedo*(gi+direct)+emissive, ACES tonemap, to the default framebuffer.
    void render(const GBuffer& gbuffer, GLuint giTex, GLuint directTex, ivec2 screenSize,
                float exposure = 1.0f);
    void destroy();
private:
    Effect fx;                        // fullscreen + composite
};

// ---- lights ---------------------------------------------------------------------------------
// Flashlight hero light. All positional data in "screen + height" space (px, px, world height).
struct Spotlight {
    vec3  pos      = {0, 0, 64};   // (px, px, height)
    vec2  dir      = {1, 0};       // screen-plane aim (will be normalized)
    float cosInner = 0.96f;        // ~16 deg half-angle
    float cosOuter = 0.86f;        // ~30 deg half-angle
    vec3  color    = {1, 1, 1};    // linear intensity
    float k1       = 0.0f;         // linear attenuation
    float k2       = 0.0005f;      // quadratic attenuation
};

class LightManager {
public:
    Spotlight flashlight;             // the single hero light (iron rule 1)
    // (future) std::vector<Spotlight> extraHeroLights;  // each costs one shadow march
};

// ---- top-level renderer ---------------------------------------------------------------------
class DeferredRenderer {
public:
    void init(ivec2 framebufferSize, const RadianceCascades::Params& rcParams = {});
    void resize(ivec2 framebufferSize);
    // Runs Pass 1..6 for the frame. `scene` enumerates entities carrying LitSprite + Motion.
    // window_size is the framebuffer pixel size; camera supplies world->screen for Pass 1.
    void draw(/*scene access via ECS registries*/ const Camera& camera, ivec2 window_size);
    LightManager lights;
private:
    GBuffer          gbuffer;
    SdfPass          sdf;
    RadianceCascades rc;
    DirectLightPass  direct;
    CompositePass    composite;
    Effect           gbufferFx;       // gbuffer.vertex + gbuffer.fragment
    GLuint           dummyVao = 0;    // bound for full-screen draws (core profile needs a VAO)
};
```

### 6.1 How the geometry pass consumes `LitSprite`

For each entity that has both `Motion` and `LitSprite`, painter-sorted by `Motion.zValue` (reusing
the existing sort in `RenderSystem::draw`):
1. `glUseProgram(gbufferFx.program)`; bind the sprite's quad VAO (created `createSprite`-style).
2. Set `transform` = `getTransform(motion).mat` (minus camera offset, exactly like the forward
   path), `projection` = `projection_2D`.
3. Bind textures: `albedo→unit0/uAlbedo`, `normal→unit1/uNormalMap` (flat-normal fallback if
   `normal` invalid), `height→unit2/uHeightMap`, `emissive→unit3/uEmissiveMap`.
4. Set scalars from the component: `uIsOccluder = isOccluder?1:0`, `uRoughness`, `uBaseHeight`,
   `uHeightRange`, `uHasHeightMap`, `uHasEmissive`, `uNormalToSurface`.
5. `glDrawElements` into the bound G-buffer FBO (4 MRT). Depth test/blend disabled; alpha-clip in
   the fragment shader handles cutouts.

The per-frame `DeferredRenderer::draw` then runs SdfPass → RadianceCascades → DirectLightPass →
CompositePass in the order of §5, threading the §3 texture units consistently.

---

## 7. Radiance-Cascade defaults (conservative, known-correct first integration)

These are the values the C++ ships with (`RadianceCascades::Params`) and the shaders assume. They
favor **correctness and stability over fidelity**; tune later.

| Parameter            | Default | Meaning / rationale                                                            |
|----------------------|---------|--------------------------------------------------------------------------------|
| `numCascades`        | **4**   | C0..C3. Enough range for a screen of a few hundred px; cheap. (Design says 4–6; start at 4.) |
| `baseProbeSpacing`   | **2 px**| Cascade-0 probe every 2 px. Spacing doubles per cascade (2,4,8,16).             |
| `baseDirCount`       | **4**   | Directions: 4, 16, 64, 256 across C0..C3 (the canonical 4× angular growth).     |
| `d0`                 | **16 px**| Cascade-0 ray length. Interval length per cascade = `d0 * 4^c` → 16,64,256,1024 px. Near = cumulative sum (0,16,80,336 px). |
| `maxSteps`           | **32**  | Sphere-trace step cap per ray; SDF makes this plenty for empty regions.         |
| `eps`                | **1.0 px**| Hit threshold against the SDF.                                                |
| `minStep`            | **0.5 px**| Minimum advance to guarantee progress near surfaces.                          |
| `halfResGI`          | **true**| `giResult` at W/2×H/2, bilinearly upsampled in composite (design §12 perf).     |
| `bilinearFix`        | **true**| Osborne–Sannikov 2024 merge; **always on** to kill ring artifacts.             |

**Interval table (derived, for C0..C3 with `d0=16`):**

| Cascade | spacing (px) | dirCount | interval len (px) | tNear (px) | tFar (px) |
|---------|--------------|----------|-------------------|------------|-----------|
| C0      | 2            | 4        | 16                | 0          | 16        |
| C1      | 4            | 16       | 64                | 16         | 80        |
| C2      | 8            | 64       | 256               | 80         | 336       |
| C3      | 16           | 256      | 1024              | 336        | 1360      |

`uIntervalNear` per cascade is set from this table by the C++ (`tNear` column); `tFar = tNear + d0*4^c`.

---

## 8. Invariants checklist (every downstream agent verifies against this)

- [ ] Every shader starts with `#version 330` and matches §4 names/locations/types **exactly**.
- [ ] Full-screen passes use `fullscreen.vertex.glsl` + their `<pass>.fragment.glsl` via
      `Effect::load_from_file(vs, fs)`; a dummy VAO is bound; `glDrawArrays(GL_TRIANGLES,0,3)`.
- [ ] G-buffer = RGBA8, RGBA8, R16F, RGBA16F at locations 0..3, with `glDrawBuffers` listing all 4.
- [ ] Normal encode/decode is `xy*0.5+0.5` / `*2-1` with `z=sqrt(1-len2(xy))≥0`, in screen+height
      space (`+x` right, `+y` up, `+z` elevation).
- [ ] Height write = `uBaseHeight + heightmap.r * uHeightRange`.
- [ ] Texture units follow §3 with NO deviation in any pass; samplers set via `glUniform1i`.
- [ ] Lighting space is `P = (gl_FragCoord.xy px, height world)`; uv = `gl_FragCoord.xy/uResolution`,
      origin bottom-left.
- [ ] The flashlight is computed ONLY in `direct_light.fragment.glsl` and is NOT in RC (iron rule 1).
- [ ] All pseudo-3D shadow/attenuation derives from `GBuffer2` height (iron rule 2).
- [ ] RC merge implements the Osborne–Sannikov bilinear fix; cascades produced top-down; C0 merge
      renders into half-res `giResult`.
- [ ] Composite = `albedo*(gi+direct)+emissive`, then Narkowicz ACES; output to FBO 0.
- [ ] No compute shaders, no image load/store, no SSBOs; every post-geometry pass is full-screen
      fragment into an FBO; JFA & RC use ping-pong FBOs.
