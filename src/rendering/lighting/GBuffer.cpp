#include "GBuffer.hpp"
#include "GlLightingUtil.hpp"
#include "render.hpp" // gl_has_errors

#include <stdexcept>

void GBuffer::init(int width, int height) {
    width_ = width;
    height_ = height;

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    // 0: albedo + occluder mask
    tex_[0] = lgl::createTexture2D(width, height, GL_RGBA8, GL_LINEAR);
    // 1: world-space normal (xyz, 0.5+0.5 encoded) + roughness
    tex_[1] = lgl::createTexture2D(width, height, GL_RGBA8, GL_NEAREST);
    // 2: height / elevation
    tex_[2] = lgl::createTexture2D(width, height, GL_R16F, GL_NEAREST);
    // 3: emissive
    tex_[3] = lgl::createTexture2D(width, height, GL_RGBA16F, GL_LINEAR);

    for (int i = 0; i < 4; ++i)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tex_[i], 0);

    // Optional depth/stencil (§12). Created so depth testing can be switched on
    // later for z-buffer sorting without re-architecting the FBO.
    glGenRenderbuffers(1, &depthRb_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRb_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRb_);

    GLenum drawBuffers[4] = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
    };
    glDrawBuffers(4, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("GBuffer FBO incomplete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl_has_errors();
}

void GBuffer::resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    lgl::resizeTexture2D(tex_[0], width, height, GL_RGBA8);
    lgl::resizeTexture2D(tex_[1], width, height, GL_RGBA8);
    lgl::resizeTexture2D(tex_[2], width, height, GL_R16F);
    lgl::resizeTexture2D(tex_[3], width, height, GL_RGBA16F);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRb_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    gl_has_errors();
}

void GBuffer::bindForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    GLenum drawBuffers[4] = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
    };
    glDrawBuffers(4, drawBuffers);
    glViewport(0, 0, width_, height_);
}

void GBuffer::clear() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    // Clear each attachment with an appropriate "empty" value.
    const float zero4[4]     = {0.f, 0.f, 0.f, 0.f};
    const float flatNormal[4]= {0.5f, 0.5f, 1.0f, 0.5f}; // +Z normal, mid roughness
    const float zero1[4]     = {0.f, 0.f, 0.f, 0.f};
    glClearBufferfv(GL_COLOR, 0, zero4);       // albedo+mask
    glClearBufferfv(GL_COLOR, 1, flatNormal);  // normal+rough
    glClearBufferfv(GL_COLOR, 2, zero1);       // height
    glClearBufferfv(GL_COLOR, 3, zero4);       // emissive
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GBuffer::destroy() {
    if (fbo_) glDeleteFramebuffers(1, &fbo_);
    glDeleteTextures(4, tex_);
    if (depthRb_) glDeleteRenderbuffers(1, &depthRb_);
    fbo_ = 0; depthRb_ = 0;
    for (int i = 0; i < 4; ++i) tex_[i] = 0;
}
