#ifndef SHADER_HPP
#define SHADER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include "include/glad/glad.h"
#include "math3d.hpp"

#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4

#define POINT_LIGHT_COUNT 1

enum EBufferType {
    INDEX_BUFFER = 0,
    POS_VB,
    TEXCOORD_VB,
    NORM_VB,
    BONE_VB,

    BUFFER_COUNT,
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
};

class ShaderProgram {
public:
    u32 mId;

    ShaderProgram(const std::string &vShaderPath, const std::string &fShaderPath);
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
