#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <cstdint>
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <streambuf>
#include <vector>
#include <string>
#include <Magick++.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define global static
#define internal static

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

#define POSITION_LOCATION 0
#define TEX_COORD_LOCATION 1
#define NORMAL_LOCATION 2
#define POINT_LIGHT_COUNT 1

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;
typedef u32 b32;

global i32 G_WWIDTH = 800;
global i32 G_WHEIGHT = 600;

enum BUFFER_TYPE {
    INDEX_BUFFER = 0,
    POS_VB       = 1,
    TEXCOORD_VB  = 2,
    NORM_VB      = 3,
    BUFFER_COUNT = 4,
};

struct MeshInfo {
    u32 NumIndices;
    u32 BaseVertex;
    u32 BaseIndex;
    u32 MaterialIndex;
};

struct Texture {
    std::string Filename;
    GLenum Target;
    GLuint Object;
    Magick::Image *Image;
    Magick::Blob Blob;

    Texture(GLenum target, const std::string& filename) {
        Target   = target;
        Filename = filename;
        Image    = NULL;
    }

    bool Load() {
        try {
            Image = new Magick::Image(Filename);
            Image->write(&Blob, "RGBA");
        } catch (Magick::Error& err) {
            std::cerr << "[Err] Failed to load texture '" << Filename << "':" << std::endl << err.what() << std::endl;
            return false;
        }

        glGenTextures(1, &Object);
        glBindTexture(Target, Object);
        glTexImage2D(Target, 0, GL_RGB, Image->columns(), Image->rows(), -0.5, GL_RGBA, GL_UNSIGNED_BYTE, Blob.data());
        glTexParameterf(Target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(Target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return true;
    }

    void Bind(GLenum textureUnit) {
        glActiveTexture(textureUnit);
        glBindTexture(Target, Object);
    }
};

struct Light {
    glm::vec3 Position;
    r32       Size;

    r32       Kc;
    r32       Kl;
    r32       Kq;

    glm::vec3 Ambient;
    glm::vec3 Diffuse;
    glm::vec3 Specular;
};

struct Camera {
    r32       FOV;
    r32       Pitch;
    r32       Yaw;
    r32       Speed;

    glm::vec3 Position;
    glm::vec3 Target;
    glm::vec3 Direction;

    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;

    Camera(float fov, float pitch, float yaw, float speed) {
        // TODO(Jovan): Extract into parameters
        FOV   = fov;
        Pitch = pitch;
        Yaw   = yaw;
        Speed = speed;

        Position     = glm::vec3(0.0f, 0.0f, 3.0f);
        glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
        Target       = glm::vec3(0.0f, 0.0f, 0.0f);
        Direction    = glm::normalize(Position - Target);
        Front        = glm::vec3(0.0f, 0.0f, -1.0f);
        Right        = glm::normalize(glm::cross(Up, Direction));
        this->Up     = glm::cross(Direction, Right);
    }
};

internal void
_ErrorCallback(int error, const char* description) {
    std::cerr << "[Err] GLFW: " << description << std::endl;
}

internal void
_KeyCallback(GLFWwindow *window, i32 key, i32 scode, i32 action, i32 mods) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

internal void
SetUniformf1(u32 programID, std::string uniform, float f) {
    glUniform1f(glGetUniformLocation(programID, uniform.c_str()), f);
}

internal void
SetUniform3fv(u32 programID, std::string uniform, const glm::vec3 &v) {
    glUniform3fv(glGetUniformLocation(programID, uniform.c_str()), 1, &v[0]);
}

internal void
SetUniform4m(u32 programID, std::string uniform, const glm::mat4 &m) {
    glUniformMatrix4fv(glGetUniformLocation(programID, uniform.c_str()), 1, GL_FALSE, &m[0][0]);
}

internal u32
LoadAndCompileShader(std::string filename, GLuint shaderType) {
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
        std::cout << "Error while compiling shader [" << shaderType << "]:" << std::endl << InfoLog << std::endl;
        return 0;
    }

    return ShaderID;
}

internal u32
CreateBasicProgram(u32 vShader, u32 fShader) {
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

    return ProgramID;
}

i32
main() {

    if(!glfwInit()) {
        std::cout << "Failed to init GLFW" << std::endl;
        return -1;
    }
    glfwSetErrorCallback(_ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow *Window = glfwCreateWindow(G_WWIDTH, G_WHEIGHT, "Frag!", 0, 0);
    if(!Window) {
        std::cerr << "[Err] GLFW: Failed creating window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetKeyCallback(Window, _KeyCallback);
    glfwMakeContextCurrent(Window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // TODO(Jovan): Move out model loading

    GLuint ModelVAO;
    GLuint ModelBuffers[BUFFER_COUNT] = {0};
    std::vector<MeshInfo> Meshes;
    std::vector<Texture*> Textures;

    std::vector<glm::vec3> Positions;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> TexCoords;
    std::vector<u32> Indices;

    glGenVertexArrays(1, &ModelVAO);
    glBindVertexArray(ModelVAO);
    glGenBuffers(ArrayCount(ModelBuffers), ModelBuffers);
    std::string Filename = "../res/models/amongus.obj";
    const aiScene *Scene = aiImportFile(Filename.c_str(), aiProcess_Triangulate |
            aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
    if(Scene) {
        Meshes.resize(Scene->mNumMeshes);
        Textures.resize(Scene->mNumMaterials);
        u32 NumVertices = 0;
        u32 NumIndices = 0;

        // NOTE(Jovan): Count vertices and indices
        for(u32 i = 0; i < Meshes.size(); ++ i) {
            Meshes[i].MaterialIndex = Scene->mMeshes[i]->mMaterialIndex;
            // NOTE(Jovan): "* 3" because we're using triangles (triangulation)
            Meshes[i].NumIndices = Scene->mMeshes[i]->mNumFaces * 3;
            Meshes[i].BaseVertex = NumVertices;
            Meshes[i].BaseVertex = NumIndices;

            NumVertices += Scene->mMeshes[i]->mNumVertices;
            NumIndices += Meshes[i].NumIndices;
        }

        // NOTE(Jovan): Allocate space
        Positions.reserve(NumVertices);
        Normals.resize(NumVertices);
        TexCoords.resize(NumVertices);
        Indices.resize(NumIndices);

        // NOTE(Jovan): Init all meshes
        for(u32 i = 0; i < Meshes.size(); ++i) {
            const aiMesh *Mesh = Scene->mMeshes[i];
            // NOTE(Jovan): Init single mesh
            const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

            // NOTE(Jovan): Populate vertex attribute vectors
            for(u32 i = 0; i < Mesh->mNumVertices; ++i){
                const aiVector3D& Pos = Mesh->mVertices[i];
                const aiVector3D& Normal = Mesh->mNormals[i];
                const aiVector3D& mTexCoords = Mesh->HasTextureCoords(0) ? Mesh->mTextureCoords[0][i] : Zero3D;

                Positions.push_back(glm::vec3(Pos.x, Pos.y, Pos.z));
                Normals.push_back(glm::vec3(Normal.x, Normal.y, Normal.z));
                TexCoords.push_back(glm::vec2(mTexCoords.x, mTexCoords.y));
            }

            // NOTE(Jovan): Populate index buffer
            for(u32 i = 0; i < Mesh->mNumFaces; ++i) {
                const aiFace& Face = Mesh->mFaces[i];
                // TODO(Jovan): Assert Face.mNumIndices == 3
                Indices.push_back(Face.mIndices[0]);
                Indices.push_back(Face.mIndices[1]);
                Indices.push_back(Face.mIndices[2]);
            }
        }

        // NOTE(Jovan): Init materials
        std::string::size_type SlashIndex = Filename.find_last_of("/");
        std::string Dir;
        if(SlashIndex == std::string::npos) {
            Dir = ".";
        } else if (SlashIndex == 0) {
            Dir = "/";
        } else {
            Dir = Filename.substr(0, SlashIndex);
        }

        for(u32 i = 0; i < Scene->mNumMaterials; ++i) {
            const aiMaterial *Material = Scene->mMaterials[i];
            Textures[i] = NULL;
            if(Material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString Path;

                if(Material->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                    std::string P(Path.data);
                    
                    if(P.substr(0, 2) == ".\\") {
                        P = P.substr(2, P.size() - 2);
                    }
                    
                    std::string FullPath = Dir + "/" + P;
                    Textures[i] = new Texture(GL_TEXTURE_2D, FullPath);
                    if(!Textures[i]->Load()) {
                        std::cerr << "[Err] Failed to load texture '" << FullPath << "'" << std::endl;
                        delete Textures[i];
                        Textures[i] = NULL;
                    } else {
                        std::cout << "Loaded texture '" << FullPath << "'\n";
                    }
                }
            }
        }

        std::cout << "Loaded " << NumIndices << " indices and " << NumVertices << "vertices" << std::endl;


        // NOTE(Jovan): Populate buffers as SOA
        glBindBuffer(GL_ARRAY_BUFFER, ModelBuffers[POS_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Positions[0]) * Positions.size(), &Positions[0], GL_STATIC_DRAW);
        glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(POSITION_LOCATION);

        glBindBuffer(GL_ARRAY_BUFFER, ModelBuffers[TEXCOORD_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(TexCoords[0]) * Positions.size(), &TexCoords[0], GL_STATIC_DRAW);
        glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(TEX_COORD_LOCATION);

        glBindBuffer(GL_ARRAY_BUFFER, ModelBuffers[NORM_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Normals[0]) * Normals.size(), &Normals[0], GL_STATIC_DRAW);
        glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(NORMAL_LOCATION);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ModelBuffers[INDEX_BUFFER]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices[0]) * Indices.size(), &Indices[0], GL_STATIC_DRAW);


    } else {
        std::cerr << "[Err] Failed to parse '" << Filename << "':" << std::endl << aiGetErrorString() << std::endl;
    }
    glBindVertexArray(0);


    // End of model loading

    u32 VShader = LoadAndCompileShader("../shaders/basic.vs", GL_VERTEX_SHADER);
    u32 FShader = LoadAndCompileShader("../shaders/basic.fs", GL_FRAGMENT_SHADER);
    u32 ProgramID = CreateBasicProgram(VShader, FShader);
    glDetachShader(ProgramID, VShader);
    glDetachShader(ProgramID, FShader);
    glDeleteShader(VShader); glDeleteShader(FShader);


    // NOTE(Jovan): Camera init
    Camera Camera(45.0f, 0.0f, -90.0f, 0.05f);

    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::mat4 View = glm::mat4(1.0f);
    View = glm::translate(View, glm::vec3(0.0f, 0.0f, -3.0f));
    View = glm::lookAt(Camera.Position, Camera.Position + Camera.Front, Camera.Up);
    glm::mat4 Model = glm::mat4(1.0f);

    // NOTE(Jovan): Set texture scale
    glUseProgram(ProgramID);
    SetUniformf1(ProgramID, "uTexScale", 1.0f);
    SetUniform4m(ProgramID, "uProjection", Projection);
    SetUniform4m(ProgramID, "uView", View);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    while(!glfwWindowShouldClose(Window)) {
        glfwGetFramebufferSize(Window, &G_WWIDTH, &G_WHEIGHT);
        r32 AspectRatio = G_WWIDTH / (float) G_WHEIGHT;
        glViewport(0, 0, G_WWIDTH, G_WHEIGHT);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        View = glm::mat4(1.0f);
        View = glm::translate(View, glm::vec3(0.0f, 0.0f, -3.0f));
        View = glm::lookAt(Camera.Position, Camera.Position + Camera.Front, Camera.Up);
        Model = glm::mat4(1.0f);
        SetUniform4m(ProgramID, "uView", View);
        SetUniform4m(ProgramID, "uModel", Model);

        // NOTE(Jovan): Render model
        glUseProgram(ProgramID);
        glBindBuffer(GL_ARRAY_BUFFER, ModelVAO);

        for(u32 i = 0; i < Meshes.size(); ++i) {
            u32 MaterialIndex = Meshes[i].MaterialIndex;
            // TODO(Jovan): assert(MaterialIndex < Textures.size());

            if(Textures[MaterialIndex]) {
                Textures[MaterialIndex]->Bind(GL_TEXTURE0);
            }

            glDrawElementsBaseVertex(GL_TRIANGLES, Meshes[i].NumIndices, GL_UNSIGNED_INT,
                    (void *) (sizeof(u32) * Meshes[i].BaseIndex), Meshes[i].BaseVertex);
        }
       
        glBindVertexArray(0);
        glUseProgram(0);


        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
