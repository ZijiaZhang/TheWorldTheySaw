#include "RadianceCascades.hpp"
#include "GlLightingUtil.hpp"
#include "render.hpp" // gl_has_errors

void RadianceCascades::init(int giWidth, int giHeight, int numCascades) {
    width_ = giWidth;
    height_ = giHeight;
    numCascades_ = numCascades;

    cascadeProgram_ = lgl::createComputeProgram(shader_path("lighting/rc_cascade.compute") + ".glsl");
    mergeProgram_   = lgl::createComputeProgram(shader_path("lighting/rc_merge.compute") + ".glsl");
    resolveProgram_ = lgl::createComputeProgram(shader_path("lighting/rc_resolve.compute") + ".glsl");

    allocTextures();
}

void RadianceCascades::allocTextures() {
    cascade_.resize(numCascades_);
    merged_.resize(numCascades_);
    for (int c = 0; c < numCascades_; ++c) {
        cascade_[c] = lgl::createTexture2D(width_, height_, GL_RGBA16F, GL_NEAREST);
        merged_[c]  = lgl::createTexture2D(width_, height_, GL_RGBA16F, GL_NEAREST);
    }
    gi_ = lgl::createTexture2D(width_, height_, GL_RGBA16F, GL_LINEAR);
    gl_has_errors();
}

void RadianceCascades::resize(int giWidth, int giHeight) {
    if (giWidth == width_ && giHeight == height_) return;
    width_ = giWidth;
    height_ = giHeight;
    for (int c = 0; c < numCascades_; ++c) {
        lgl::resizeTexture2D(cascade_[c], width_, height_, GL_RGBA16F);
        lgl::resizeTexture2D(merged_[c],  width_, height_, GL_RGBA16F);
    }
    lgl::resizeTexture2D(gi_, width_, height_, GL_RGBA16F);
}

void RadianceCascades::compute(GLuint sdfTex, GLuint emissiveTex) {
    const int gx = lgl::groups(width_, 8);
    const int gy = lgl::groups(height_, 8);

    // ---- 1. Per-cascade interval ray-march ----
    glUseProgram(cascadeProgram_);
    glUniform2i(glGetUniformLocation(cascadeProgram_, "uSize"), width_, height_);
    glUniform1i(glGetUniformLocation(cascadeProgram_, "uBaseSpacing"), kBaseSpacing);
    glUniform1f(glGetUniformLocation(cascadeProgram_, "uBaseInterval"), baseInterval);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sdfTex);
    glUniform1i(glGetUniformLocation(cascadeProgram_, "uSdf"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, emissiveTex);
    glUniform1i(glGetUniformLocation(cascadeProgram_, "uEmissive"), 1);
    GLint cascLoc = glGetUniformLocation(cascadeProgram_, "uCascade");
    for (int c = 0; c < numCascades_; ++c) {
        glUniform1i(cascLoc, c);
        glBindImageTexture(0, cascade_[c], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(gx, gy, 1);
    }
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    gl_has_errors();

    // ---- 2. Merge top-down (bilinear fix) ----
    glUseProgram(mergeProgram_);
    glUniform2i(glGetUniformLocation(mergeProgram_, "uSize"), width_, height_);
    glUniform1i(glGetUniformLocation(mergeProgram_, "uBaseSpacing"), kBaseSpacing);
    glUniform1i(glGetUniformLocation(mergeProgram_, "uBilinearFix"), bilinearFix ? 1 : 0);
    GLint mCascLoc  = glGetUniformLocation(mergeProgram_, "uCascade");
    GLint mUpperLoc = glGetUniformLocation(mergeProgram_, "uHasUpper");
    for (int c = numCascades_ - 1; c >= 0; --c) {
        glUniform1i(mCascLoc, c);
        glUniform1i(mUpperLoc, (c == numCascades_ - 1) ? 0 : 1);
        glBindImageTexture(0, cascade_[c], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        if (c == numCascades_ - 1)
            glBindImageTexture(1, cascade_[c], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F); // unused
        else
            glBindImageTexture(1, merged_[c + 1], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        glBindImageTexture(2, merged_[c], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(gx, gy, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    gl_has_errors();

    // ---- 3. Resolve C0 -> per-pixel irradiance ----
    glUseProgram(resolveProgram_);
    glUniform2i(glGetUniformLocation(resolveProgram_, "uSize"), width_, height_);
    glUniform1i(glGetUniformLocation(resolveProgram_, "uBaseSpacing"), kBaseSpacing);
    glUniform1f(glGetUniformLocation(resolveProgram_, "uGain"), gain);
    glBindImageTexture(0, merged_[0], 0, GL_FALSE, 0, GL_READ_ONLY,  GL_RGBA16F);
    glBindImageTexture(1, gi_,        0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    gl_has_errors();
}

void RadianceCascades::destroy() {
    if (cascadeProgram_) glDeleteProgram(cascadeProgram_);
    if (mergeProgram_)   glDeleteProgram(mergeProgram_);
    if (resolveProgram_) glDeleteProgram(resolveProgram_);
    if (!cascade_.empty()) glDeleteTextures((GLsizei)cascade_.size(), cascade_.data());
    if (!merged_.empty())  glDeleteTextures((GLsizei)merged_.size(),  merged_.data());
    if (gi_) glDeleteTextures(1, &gi_);
    cascade_.clear();
    merged_.clear();
    gi_ = 0;
    cascadeProgram_ = mergeProgram_ = resolveProgram_ = 0;
}
