#include "framebuffer_gl.hpp"

FramebufferGL::FramebufferGL(const v2 &size) : mSize(size) {
  create(&mId, &mRBO, &mTexture, mSize.X, mSize.Y);
}

FramebufferGL::FramebufferGL(u32 width, u32 height) : mSize(width, height) {
  create(&mId, &mRBO, &mTexture, mSize.X, mSize.Y);
}

void FramebufferGL::Resize(r32 newWidth, r32 newHeight) {
  u32 OldFBO = mId;
  u32 OldFBOTexture = mTexture;
  u32 OldRBO = mRBO;
  create(&mId, &mRBO, &mTexture, newWidth, newHeight);
  glDeleteFramebuffers(1, &OldFBO);
  glDeleteRenderbuffers(1, &OldRBO);
  glDeleteTextures(1, &OldFBOTexture);
}

void FramebufferGL::Bind(u32 fbo, const v4 &clearColor, GLbitfield clearMask,
                         const v2 &viewportSize) {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glEnable(GL_DEPTH_TEST);
  glClearColor(clearColor.X, clearColor.Y, clearColor.Z, clearColor.W);
  glClear(clearMask);
  glViewport(0, 0, viewportSize.X, viewportSize.Y);
}

void FramebufferGL::create(u32 *fbo, u32 *rbo, u32 *texture, r32 width,
                           r32 height) {
  // TODO(Jovan): Tidy up
  glGenFramebuffers(1, fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

  glGenTextures(1, texture);
  glBindTexture(GL_TEXTURE_2D, *texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, (void *)0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         *texture, 0);

  glGenRenderbuffers(1, rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, *rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, *rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  // TODO(Jovan): Check for concrete errors
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "[Err] Framebuffer not complete" << std::endl;
  }

  mSize = v2(width, height);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
