#pragma once
//
// A self-contained demo scene that exercises the deferred lighting pipeline:
// a textured floor, brick walls and rounded props (occluders with height), an
// emissive lamp feeding the Radiance Cascades, plus a mouse-driven flashlight
// spotlight that casts height-field shadows.
//
// Activated from main(); RenderSystem drives update()/the projection each frame.
//
#include "common.hpp"
#include "LightingComponents.hpp"

class LightingDemo {
public:
    // Build textures + scene + lights. Call once at startup.
    static void setup(vec2 screenSize);

    // Re-spawn the scene after a level reload wiped the Motion-bearing lit
    // sprites (clears existing demo lights first so they don't accumulate).
    static void respawn();

    // Aim the flashlight at the mouse (mouse in GLFW top-left pixel coords).
    static void update(vec2 mouseTopLeft, vec2 focus, vec2 screenSize, const ObliqueProjection& proj);

    // Number keys 0..8 pick a debug view; B toggles the RC bilinear fix.
    static void onKey(int key, int action);

    static bool active;
    static int  debugMode;
    static bool bilinearFixToggle;       // mirror; applied by RenderSystem
    static ObliqueProjection projection; // canonical projection for the scene
};
