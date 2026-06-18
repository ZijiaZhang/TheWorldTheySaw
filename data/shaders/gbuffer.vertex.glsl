#version 330

// Geometry pass (Pass 1) vertex shader.
// Uses the template's attribute names (in_position / in_texcoord) so the
// existing attribute binding in the renderer applies unchanged.

in vec3 in_position;
in vec2 in_texcoord;

out vec2 vUV;

uniform mat3 transform;     // per-sprite model transform (Transform::mat), camera-relative
uniform mat3 projection;    // 2D screen projection (projection_2D from RenderSystem::draw)

void main()
{
    vUV = in_texcoord;
    vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}
