#ifndef SHADER_HPP
#define SHADER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include "include/glad/glad.h"
#include "math3d.hpp"

#define POINT_LIGHT_COUNT 1

struct Light {
    r32 Size;
    r32 Kc;
    r32 Kl;
    r32 Kq;

    v3 Position;
    v3 Ambient;
    v3 Diffuse;
    v3 Specular;
};

struct GLBuffers {
    enum EBufferType {
        INDEX_BUFFER = 0,
        POS_VB,
        TEXCOORD_VB,
        NORM_VB,
        BONE_VB,

        BUFFER_COUNT,
    };

    enum EBufferLocations {
        POSITION_LOCATION = 0,
        TEXCOORD_LOCATION,
        NORMAL_LOCATION,
        BONE_ID_LOCATION,
        BONE_WEIGHT_LOCATION
    };

    u32 mIds[BUFFER_COUNT];

    GLBuffers();
    void Destroy();
    void BufferData(EBufferType type, GLsizeiptr size, const void *data, GLenum target = GL_ARRAY_BUFFER, GLenum usage = GL_STATIC_DRAW);
    void SetPointer(EBufferType bufferType, GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer, GLboolean normalized = GL_FALSE, GLenum target = GL_ARRAY_BUFFER);
    void SetIPointer(EBufferType bufferType, GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer, GLenum target = GL_ARRAY_BUFFER);
    
};

struct Texture {
    enum ETextureType {
        DIFFUSE = 0,
        SPECULAR,

        TYPECOUNT
    };

    u32          mId;
    ETextureType mType;
    std::string  mPath;
    Texture(const std::string &path, ETextureType type);
    ~Texture();
};

class ShaderProgram {
public:
    u32 mId;

    ShaderProgram(const std::string &vShaderPath, const std::string &fShaderPath);
    void SetPointLight(const Light &light, u32 index);
    void SetUniform1i(const std::string &uniform, i32 i) const;
    void SetUniform1f(const std::string &uniform, r32 f) const;
    void SetUniform3f(const std::string &uniform, const v3 &v) const;
    void SetUniform4m(const std::string &uniform, const m44 &m, GLboolean transpose = GL_FALSE) const;
    void SetUniform4m(const std::string &uniform, const std::vector<m44> &m, GLboolean transpose = GL_FALSE) const;
private:
    u32 loadAndCompileShader(std::string filename, GLuint shaderType);
    u32 createBasicProgram(u32 vShader, u32 fShader);
};

#endif
