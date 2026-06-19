# Forest Gate Rendering Gaps

This scene is implemented with the current pseudo-3D oblique sprite renderer:
alpha sprites feed the deferred G-buffer, each sprite can act as a floor or
upright surface, and lights use the existing height-field shadow pipeline.

## Supported In This Pass

- Reusable PNG sprites for every major scene element.
- Alpha-cutout props, terrain patches, and rain overlay.
- Oblique pseudo-3D placement using `ObliqueProjection`.
- Painter-sorted sprite layering through `LitSprite::sortKey`.
- Dynamic point and spot lights for the booth lamp, bus taillights, moon fill,
  and player flashlight.
- Mouse-aimed player flashlight.
- Player movement with WASD or arrow keys.
- Occluder masks for large upright sprites so the existing SDF and shadow passes
  can treat them as blockers.
- Neutral fallback normals for image-tool sprites. This avoids the incorrect
  result of treating a complex painted prop as one single vertical wall plane.
- Runtime silhouette height masks derived from sprite alpha, so props can cast
  approximate object-shaped height-field shadows.

## Not Yet Supported By The Renderer

- True planar reflections. Puddle and wet-road highlights are painted into the
  sprite art; the renderer does not mirror scene geometry or lights across water.
- Per-sprite authored normal and height maps from the image tool. The current
  scene uses generated albedo sprites plus the renderer's flat +Z normal fallback
  and alpha-derived silhouette height. Adding authored material maps would improve
  grazing light, contact detail, and shadow shape.
- Bottom-anchored sprite placement. `LitSprite` anchors quads at their center, so
  large upright props need hand-tuned world positions and scale. A bottom-anchor
  option would make reusable character/prop placement easier.
- True depth buffering between overlapping sprites. Ordering is painter-sort
  based, not per-pixel depth, so complex interpenetration still needs careful
  sprite layering.
- Animated sprite states for the player. The player is movable, but the current
  asset is a single reading pose rather than a directional walk cycle.
- Runtime text rendering for the chapter title and prompts. The image-generated
  title sprite is not displayed because the glyphs were not reliable enough;
  exact in-game localization needs a font/text pass or manually approved title
  art.
- Perfect AI-rendered Chinese glyph fidelity. The large sign is readable, but
  one warning character is stylized; production art should replace it with a
  manually approved localized sprite if exact signage is required.
