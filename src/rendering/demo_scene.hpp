#pragma once

// Populates a small programmatic scene that exercises the deferred lighting pipeline:
// a floor, several normal-mapped occluders at different heights (so they cast 3D-feel
// height shadows), and an emissive blob (fed into Radiance Cascades GI). All textures
// are generated procedurally, so the template ships with no binary art assets.
//
// Intended to be wired to WorldSystem::post_restart so it survives the R reload.
void setupLightingDemo();
