#include "SdfPass.hpp"
#include "GlLightingUtil.hpp"
#include "render.hpp" // gl_has_errors

#include <algorithm>
#include <cmath>

void SdfPass::init(int width, int height) {
    width_ = width;
    height_ = height;

    seedProgram_     = lgl::createComputeProgram(shader_path("lighting/sdf_seed.compute") + ".glsl");
    jfaProgram_      = lgl::createComputeProgram(shader_path("lighting/sdf_jfa.compute") + ".glsl");
    distanceProgram_ = lgl::createComputeProgram(shader_path("lighting/sdf_distance.compute") + ".glsl");

    seedA_ = lgl::createTexture2D(width, height, GL_RG32F, GL_NEAREST);
    seedB_ = lgl::createTexture2D(width, height, GL_RG32F, GL_NEAREST);
    sdf_   = lgl::createTexture2D(width, height, GL_R16F, GL_LINEAR);
    gl_has_errors();
}

void SdfPass::resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    lgl::resizeTexture2D(seedA_, width, height, GL_RG32F);
    lgl::resizeTexture2D(seedB_, width, height, GL_RG32F);
    lgl::resizeTexture2D(sdf_,   width, height, GL_R16F);
}

void SdfPass::generate(GLuint occluderMaskTex) {
    const int gx = lgl::groups(width_, 8);
    const int gy = lgl::groups(height_, 8);

    // ---- Seed: occluder pixels store their own coords, others a sentinel ----
    glUseProgram(seedProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, occluderMaskTex);
    glUniform1i(glGetUniformLocation(seedProgram_, "uMask"), 0);
    glUniform2i(glGetUniformLocation(seedProgram_, "uSize"), width_, height_);
    glBindImageTexture(0, seedA_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    gl_has_errors();

    // ---- Jump flood: step = N/2, N/4, ..., 1 ----
    GLuint readTex = seedA_;
    GLuint writeTex = seedB_;
    int maxDim = std::max(width_, height_);
    int step = 1;
    while (step < maxDim) step <<= 1;
    step >>= 1; // largest power of two < maxDim

    glUseProgram(jfaProgram_);
    glUniform2i(glGetUniformLocation(jfaProgram_, "uSize"), width_, height_);
    GLint stepLoc = glGetUniformLocation(jfaProgram_, "uStep");
    for (; step >= 1; step >>= 1) {
        glUniform1i(stepLoc, step);
        glBindImageTexture(0, readTex,  0, GL_FALSE, 0, GL_READ_ONLY,  GL_RG32F);
        glBindImageTexture(1, writeTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
        glDispatchCompute(gx, gy, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        std::swap(readTex, writeTex);
    }
    // Latest result is in readTex; make seedA_ canonical for debug/seed().
    if (readTex != seedA_) std::swap(seedA_, seedB_);
    gl_has_errors();

    // ---- Distance: |pixel - nearestSeed| ----
    glUseProgram(distanceProgram_);
    glUniform2i(glGetUniformLocation(distanceProgram_, "uSize"), width_, height_);
    glBindImageTexture(0, seedA_, 0, GL_FALSE, 0, GL_READ_ONLY,  GL_RG32F);
    glBindImageTexture(1, sdf_,   0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    gl_has_errors();
}

void SdfPass::destroy() {
    if (seedProgram_)     glDeleteProgram(seedProgram_);
    if (jfaProgram_)      glDeleteProgram(jfaProgram_);
    if (distanceProgram_) glDeleteProgram(distanceProgram_);
    GLuint texs[3] = {seedA_, seedB_, sdf_};
    glDeleteTextures(3, texs);
    seedA_ = seedB_ = sdf_ = 0;
    seedProgram_ = jfaProgram_ = distanceProgram_ = 0;
}
