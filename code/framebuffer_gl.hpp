#ifndef FRAMEBUFFER_GL_HPP
#define FRAMEBUFFER_GL_HPP

#include <iostream>
#include "include/glad/glad.h"
#include "math3d.hpp"

class FramebufferGL {
private:
    void create(u32 *fbo, u32 *rbo, u32 *texture, r32 width, r32 height);
public:
    v2  mSize;
    u32 mId;
    u32 mRBO;
    u32 mTexture;

    FramebufferGL(const v2 &size);
    FramebufferGL(u32 width, u32 height);
    void Resize(r32 newWidth, r32 newHeight);
    static void Bind(u32 fbo, const v4 &clearColor, GLbitfield clearMask, const v2 &viewportSize);
};

#endif