#include "shader.hpp"


#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

GLBuffers::GLBuffers() {
    glGenBuffers(BUFFER_COUNT, mIds);
}

void
GLBuffers::Destroy() {
    glDeleteBuffers(BUFFER_COUNT, mIds);
}

void
GLBuffers::SetPointer(EBufferType bufferType, GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer, GLboolean normalized, GLenum target){
    glBindBuffer(target, mIds[bufferType]);
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    glEnableVertexAttribArray(index);
    glBindBuffer(target, 0);
}

void
GLBuffers::BufferData(EBufferType type, GLsizeiptr size, const void *data, GLenum target, GLenum usage) {
    glBindBuffer(target, mIds[type]);
    glBufferData(target, size, data, usage);
    glBindBuffer(target, 0);
}

Texture::Texture(const std::string &path, ETextureType type) {
    mPath = path;
    mType = type;
    std::replace(mPath.begin(), mPath.end(), '\\', '/');
    glGenTextures(1, &mId);
    i32 Width, Height, Channels;
    unsigned char *Data = stbi_load(mPath.c_str(), &Width, &Height, &Channels, 0);
    std::cout << "Loading " << mPath << " image" << std::endl;
    if(Data) {
        GLenum Format;
        if(Channels == 1) {
            Format = GL_RED;
        } else if (Channels == 3) {
            Format = GL_RGB;
        } else if (Channels == 4) {
            Format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, mId);
        glTexImage2D(GL_TEXTURE_2D, 0, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        stbi_image_free(Data);
    } else {
        std::cerr << "[Err] Texture failed to load" << std::endl;
    }
}

Texture::~Texture() {
    // glDeleteTextures(1, &mId);
}

ShaderProgram::ShaderProgram(const std::string &vShaderPath, const std::string &fShaderPath) {
    u32 vs = loadAndCompileShader(vShaderPath, GL_VERTEX_SHADER);
    u32 fs = loadAndCompileShader(fShaderPath, GL_FRAGMENT_SHADER);
    mId = createBasicProgram(vs, fs);
}

u32
ShaderProgram::loadAndCompileShader(std::string filename, GLuint shaderType) {
    u32 ShaderID = 0;
    std::ifstream In(filename);
    std::string Str;

    In.seekg(0, std::ios::end);
    Str.reserve(In.tellg());
    In.seekg(0, std::ios::beg);

    Str.assign((std::istreambuf_iterator<char>(In)), std::istreambuf_iterator<char>());
    const char *CharContent = Str.c_str();

    ShaderID = glCreateShader(shaderType);
    glShaderSource(ShaderID, 1, &CharContent, NULL);
    glCompileShader(ShaderID);

    int Success;
    char InfoLog[512];
    glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Success);
    if (!Success) {
        glGetShaderInfoLog(ShaderID, 256, NULL, InfoLog);
        std::string ShaderTypeName = shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment";
        std::cout << "Error while compiling shader [" << ShaderTypeName << "]:" << std::endl << InfoLog << std::endl;
        return 0;
    }

    return ShaderID;
}

u32
ShaderProgram::createBasicProgram(u32 vShader, u32 fShader) {
    u32 ProgramID = 0;
    ProgramID = glCreateProgram();
    glAttachShader(ProgramID, vShader);
    glAttachShader(ProgramID, fShader);
    glLinkProgram(ProgramID);

    i32 Success;
    char InfoLog[512];
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Success);
    if(!Success) {
        glGetProgramInfoLog(ProgramID, 512, NULL, InfoLog);
        std::cerr << "[Err] Failed to link shader program:" << std::endl << InfoLog << std::endl;
        return 0;
    }

    glDetachShader(ProgramID, vShader);
    glDetachShader(ProgramID, fShader);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return ProgramID;
}

void
ShaderProgram::SetUniform1i(const std::string &uniform, i32 i) const {
    glUniform1i(glGetUniformLocation(mId, uniform.c_str()), i);
}


void
ShaderProgram::SetUniform1f(const std::string &uniform, r32 f) const {
    glUniform1f(glGetUniformLocation(mId, uniform.c_str()), f);
}

void
ShaderProgram::SetUniform3f(const std::string &uniform, const v3 &v) const {
    glUniform3fv(glGetUniformLocation(mId, uniform.c_str()), 1, &v[0]);
}

void
ShaderProgram::SetUniform4m(const std::string &uniform, const m44 &m, GLboolean transpose) const {
    glUniformMatrix4fv(glGetUniformLocation(mId, uniform.c_str()), 1, transpose, &m[0][0]);
}

void
ShaderProgram::SetUniform4m(const std::string &uniform, const std::vector<m44> &m, GLboolean transpose) const {
    glUniformMatrix4fv(glGetUniformLocation(mId, uniform.c_str()), m.size(), transpose, (GLfloat*)&m[0]);
}
