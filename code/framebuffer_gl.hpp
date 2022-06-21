#ifndef FRAMEBUFFER_GL_HPP
#define FRAMEBUFFER_GL_HPP

#include <iostream>
#include "include/glad/glad.h"
#include "math3d.hpp"

struct FramebufferGL {
    v2  mSize;
    u32 mId;
    u32 mRBO;
    u32 mTexture;

    FramebufferGL(const v2 &size);
    FramebufferGL(u32 width, u32 height);
    void Resize(const v2 &newSize);
    void create(u32 *fbo, u32 *rbo, u32 *texture, u32 width, u32 height);
};

#endif