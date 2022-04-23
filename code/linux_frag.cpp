#include <iostream>
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>
#include <algorithm>

#include "frag.hpp"
#include "types.hpp"
#include "ui.hpp"
#include "shader.hpp"
#include "model.hpp"

internal i32 _WindowWidth = 800;
internal i32 _WindowHeight = 600;

void
_CreateFramebuffer(u32 *fbo, u32 *rbo, u32 *texture, i32 width, i32 height) {
    // TODO(Jovan): Tidy up
    glGenFramebuffers(1, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, (void*)0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0);

    glGenRenderbuffers(1, rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, *rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *rbo);

    // TODO(Jovan): Check for concrete errors
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[Err] Framebuffer not complete" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

internal void
_ErrorCallback(int error, const char* description) {
    std::cerr << "[Err] GLFW: " << description << std::endl;
}

internal void
_FramebufferSizeCallback(GLFWwindow *window, i32 width, i32 height) {
    EngineState *State = (EngineState*) glfwGetWindowUserPointer(window);
    _WindowWidth = width;
    _WindowHeight = height;
}

internal void
_KeyCallback(GLFWwindow *window, i32 key, i32 scode, i32 action, i32 mods) {
    EngineState *State = (EngineState*)glfwGetWindowUserPointer(window);

    if(key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
        State->mDebugBoneIdx = (State->mDebugBoneIdx + 1) % 34;
    }

    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

internal void
_CursorPosCallback(GLFWwindow *window, r64 xNew, r64 yNew) {
    EngineState *State = (EngineState*)glfwGetWindowUserPointer(window);
}

internal void
_MouseButtonCallback(GLFWwindow *window, i32 button, i32 action, i32 mods) {
    EngineState *State = (EngineState*)glfwGetWindowUserPointer(window);
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        State->mLeftMouse = true;
    }
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        State->mLeftMouse = false;
    }
}

internal void
_ScrollCallback(GLFWwindow *window, r64 xoffset, r64 yoffset) {
    EngineState *State = (EngineState*)glfwGetWindowUserPointer(window);
    if(State->mSceneWindowFocused) {
        State->mCamera->Zoom(yoffset, State->mDT);
    }
}

internal inline r64
_CurrentTimeInMillis() {
    return glfwGetTime() * 1000.0;
}

void
RenderModel(Model &model, ShaderProgram &program, r32 runningTime, u32 vao, u32 indexBuffer) {
    model.mModel.LoadIdentity();
    model.mModel
        .Translate(model.mPosition)
        // TODO(Jovan): Tidy it up with quaternions?
        .Rotate(quat(v3(1.0f, 0.0f, 0.0f), model.mRotation.X))
        .Rotate(quat(v3(0.0f, 1.0f, 0.0f), model.mRotation.Y))
        .Rotate(quat(v3(0.0f, 0.0f, 1.0f), model.mRotation.Z))
        .Scale(model.mScale);
    program.SetUniform4m("uModel", model.mModel);
    program.SetUniform4m("uBones", model.mBoneInfos.mFinalTransforms);

    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    for(u32 MeshIdx = 0; MeshIdx < model.mMeshes.size(); ++MeshIdx) {
        const Mesh &Mesh = model.mMeshes[MeshIdx];
        const Material &Material = Mesh.mMaterial;

        if(Material.mDiffuseTextureId) {
            glActiveTexture(GL_TEXTURE0);
            program.SetUniform1i("uDiffuse", 0);
            glBindTexture(GL_TEXTURE_2D, Material.mDiffuseTextureId);
        }

        if(Material.mSpecularTextureId) {
            glActiveTexture(GL_TEXTURE1);
            program.SetUniform1i("uSpecular", 1);
            glBindTexture(GL_TEXTURE_2D, Material.mSpecularTextureId);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES,
                model.mMeshes[MeshIdx].mNumIndices,
                GL_UNSIGNED_INT,
                (void*)(sizeof(u32) * model.mMeshes[MeshIdx].mBaseIndex),
                model.mMeshes[MeshIdx].mBaseVertex);

        glActiveTexture(GL_TEXTURE0);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

i32
main() {

    if(!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return -1;
    }
    glfwSetErrorCallback(_ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow *Window = glfwCreateWindow(_WindowWidth, _WindowHeight, "Frag!", 0, 0);
    if(!Window) {
        std::cerr << "[Err] GLFW: Failed creating window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(Window, _FramebufferSizeCallback);
    glfwSetKeyCallback(Window, _KeyCallback);
    glfwSetMouseButtonCallback(Window, _MouseButtonCallback);
    glfwSetCursorPosCallback(Window, _CursorPosCallback);
    glfwSetScrollCallback(Window, _ScrollCallback);

    glfwMakeContextCurrent(Window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE(Jovan): Init imgui
    InitUI(Window);

    ShaderProgram Phong("../shaders/basic.vert", "../shaders/basic.frag");
    ShaderProgram RiggedPhong("../shaders/rigged.vert", "../shaders/rigged.frag");
    ShaderProgram Debug("../shaders/debug.vert", "../shaders/debug.frag");

    Model Dragon("../res/boblampclean.md5mesh");
    
    Dragon.mPosition = v3(0.0f);
    Dragon.mRotation = v3(-90.0f, 0.0f, 0.0f);
    Dragon.mScale    = v3(0.08f);

    u32 ModelVAO;
    std::vector<v2> TexCoords;
    std::vector<v3> Vertices;
    std::vector<v3> Normals;
    std::vector<u32> Indices;
    std::vector<Texture> ModelTextures;
    std::vector<m44> BoneTransforms;

    if(!Dragon.Load(Vertices, Normals, TexCoords, Indices)) {
        std::cerr << "[err] failed to load " << Dragon.mFilepath << std::endl;
    }
    Dragon.LoadBoneTransforms(BoneTransforms);

    glGenVertexArrays(1, &ModelVAO);
    glBindVertexArray(ModelVAO);
    GLBuffers ModelBuffers;
    ModelBuffers.BufferData(GLBuffers::POS_VB, sizeof(Vertices[0]) * Vertices.size(), &Vertices[0]);
    ModelBuffers.SetPointer(GLBuffers::POS_VB, GLBuffers::POSITION_LOCATION, 3, GL_FLOAT, 0, 0);

    ModelBuffers.BufferData(GLBuffers::NORM_VB, sizeof(Normals[0]) * Normals.size(), &Normals[0]);
    ModelBuffers.SetPointer(GLBuffers::NORM_VB, GLBuffers::NORMAL_LOCATION, 3, GL_FLOAT, 0, 0);

    ModelBuffers.BufferData(GLBuffers::TEXCOORD_VB, sizeof(TexCoords[0]) * TexCoords.size(), &TexCoords[0]);
    ModelBuffers.SetPointer(GLBuffers::TEXCOORD_VB, GLBuffers::TEXCOORD_LOCATION, 2, GL_FLOAT, 0, 0);

    ModelBuffers.BufferData(GLBuffers::BONE_VB, sizeof(VertexBoneData) * Dragon.mBones.size(), &Dragon.mBones[0]);
    ModelBuffers.SetIPointer(GLBuffers::BONE_VB, GLBuffers::BONE_ID_LOCATION, NUM_BONES_PER_VERTEX, GL_INT, sizeof(VertexBoneData), 0);
    ModelBuffers.SetPointer(GLBuffers::BONE_VB, GLBuffers::BONE_WEIGHT_LOCATION, NUM_BONES_PER_VERTEX, GL_FLOAT, sizeof(VertexBoneData),
           (const GLvoid*)(NUM_BONES_PER_VERTEX * sizeof(u32)));

    ModelBuffers.BufferData(GLBuffers::INDEX_BUFFER, sizeof(Indices[0]) * Indices.size(), &Indices[0], GL_ELEMENT_ARRAY_BUFFER);
    glBindVertexArray(0);

    for(u32 MeshIdx = 0; MeshIdx < Dragon.mMeshes.size(); ++MeshIdx) {
        Material &Mat = Dragon.mMeshes[MeshIdx].mMaterial;
        std::vector<Texture>::const_iterator Begin = ModelTextures.begin();
        std::vector<Texture>::const_iterator End = ModelTextures.end();
        // TODO(Jovan): Maybe avoid lambdas?
        auto CustomLambda = [&](const std::string &str1) { return [&](const Texture &t) { return t.mPath == str1; }; };
        if(!Mat.mDiffusePath.empty() && std::find_if(Begin, End, CustomLambda(Mat.mDiffusePath)) == End) {
            Texture Diffuse(Mat.mDiffusePath, Texture::DIFFUSE);
            Mat.mDiffuseTextureId = Diffuse.mId;
            ModelTextures.push_back(Diffuse);
        } else if(!Mat.mSpecularPath.empty() && std::find_if(Begin, End, CustomLambda(Mat.mSpecularPath)) == End) {
            Texture Specular(Mat.mSpecularPath, Texture::SPECULAR);
            Mat.mSpecularTextureId = Specular.mId;
            ModelTextures.push_back(Specular);
        }
    }

    // NOTE(Jovan): Camera init
    Camera OrbitalCamera(45.0f, 2.0f);
    EngineState State(&OrbitalCamera);
    glfwSetWindowUserPointer(Window, &State);

    State.mProjection = perspective(State.mCamera->mFOV, State.mFramebufferSize.X / (r32) State.mFramebufferSize.Y, 0.1f, 100.0f);
    m44 View(1.0f);
    View = lookAt(State.mCamera->mPosition, State.mCamera->mTarget, State.mCamera->mUp);

    // NOTE(Jovan): Set texture scale
    glUseProgram(Phong.mId);
    Phong.SetUniform1f("uTexScale", 1.0f);

    Light PointLight;
    PointLight.Ambient = v3(0.3f);
    PointLight.Diffuse = v3(0.5f);
    PointLight.Specular = v3(1.0f);
    PointLight.Kc = 1.0f;
    PointLight.Kl = 0.09f;
    PointLight.Kq = 0.032f;
    PointLight.Position = v3(0.0f, 1.0f, 2.0f);
    Phong.SetPointLight(PointLight, 0);

    glUseProgram(RiggedPhong.mId);
    RiggedPhong.SetUniform1f("uTexScale", 1.0f);
    RiggedPhong.SetPointLight(PointLight, 0);
    glUseProgram(Phong.mId);

    u32 FBO, RBO;
    _CreateFramebuffer(&FBO, &RBO, &State.mFBOTexture, State.mFramebufferSize.X, State.mFramebufferSize.Y);

    r32 StartTimeMillis = _CurrentTimeInMillis();
    r32 EndTimeMillis = _CurrentTimeInMillis();
    r32 BeginTimeMillis = _CurrentTimeInMillis();
    r32 RunningTimeMillis = _CurrentTimeInMillis();
    State.mDT = EndTimeMillis - StartTimeMillis;

    u32 OldFBO;
    u32 OldRBO;
    u32 OldFBOTexture;

    ImGuiWindowFlags MainWindowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoFocusOnAppearing;
    MainWindow Main("Main", MainWindowFlags);
    SceneWindow Scene("Scene", ImGuiWindowFlags_AlwaysAutoResize);
    ModelWindow ModelWindow("Model", ImGuiWindowFlags_AlwaysAutoResize);

    glEnable(GL_TEXTURE_2D);
    while(!glfwWindowShouldClose(Window)) {

        StartTimeMillis = _CurrentTimeInMillis();
        RunningTimeMillis = _CurrentTimeInMillis() - BeginTimeMillis;
        
        glUseProgram(Phong.mId);
        if(Scene.mHasResized) {
            OldFBO = FBO;
            OldFBOTexture = State.mFBOTexture;
            OldRBO = RBO;

            _CreateFramebuffer(&FBO, &RBO, &State.mFBOTexture, State.mFramebufferSize.X, State.mFramebufferSize.Y);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            State.mProjection = perspective(State.mCamera->mFOV, State.mFramebufferSize.X / (r32) State.mFramebufferSize.Y, 0.1f, 100.0f);

            glDeleteFramebuffers(1, &OldFBO);
            glDeleteRenderbuffers(1, &OldRBO);
            glDeleteTextures(1, &OldFBOTexture);
            Scene.mHasResized = false;
        }

        Phong.SetUniform4m("uProjection", State.mProjection);
        Phong.SetUniform3f("uViewPos", State.mCamera->mPosition);

        View.LoadIdentity();
        View = lookAt(State.mCamera->mPosition, State.mCamera->mTarget, State.mCamera->mUp);
        Phong.SetUniform4m("uView", View);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0x34 / (r32) 255, 0x49 / (r32) 255, 0x5e / (r32) 255, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, State.mFramebufferSize.X, State.mFramebufferSize.Y);

        // glUseProgram(Debug.mId);
        // Debug.SetUniform4m("uProjection", State.mProjection);
        // Debug.SetUniform3f("uViewPos", State.mCamera->mPosition);
        // Debug.SetUniform4m("uView", View);
        // Debug.SetUniform1i("uDisplayBoneIdx", State.mDebugBoneIdx);

        glUseProgram(RiggedPhong.mId);
        RiggedPhong.SetUniform4m("uProjection", State.mProjection);
        RiggedPhong.SetUniform3f("uViewPos", State.mCamera->mPosition);
        RiggedPhong.SetUniform4m("uView", View);
        RiggedPhong.SetUniform1i("uDisplayBoneIdx", State.mDebugBoneIdx);

        // NOTE(Jovan): Render model
        RenderModel(Dragon, RiggedPhong, RunningTimeMillis, ModelVAO, ModelBuffers.mIds[GLBuffers::INDEX_BUFFER]);
        glUseProgram(Phong.mId);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, _WindowWidth, _WindowHeight);
        glUseProgram(0);

        NewFrameUI();

        Main.Render(&State, _WindowWidth, _WindowHeight);
        Scene.Render(&State);
        ModelWindow.Render(Dragon.mFilepath, &Dragon.mPosition[0], &Dragon.mRotation[0] ,&Dragon.mScale[0], Dragon.mNumVertices);

        ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Position: %.2f, %.2f, %.2f", State.mCamera->mPosition.X, State.mCamera->mPosition.Y, State.mCamera->mPosition.Z);
        ImGui::Text("Pitch: %.2f", State.mCamera->mPitch * 180.0f / PI);
        ImGui::Text("Yaw: %.2f", State.mCamera->mYaw * 180.0f / PI);
        ImGui::End();

        RenderUI();

        EndTimeMillis = _CurrentTimeInMillis();
        State.mDT = EndTimeMillis - StartTimeMillis;

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    DisposeUI();
    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
