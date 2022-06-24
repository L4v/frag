#include <cassert>
#include <iostream>
#include "framebuffer_gl.hpp"
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>
#include <algorithm>

#include "frag.hpp"
#include "types.hpp"
#include "ui.hpp"
#include "shader.hpp"
#include "model.hpp"

static const i32 DefaultWindowWidth = 800;
static const i32 DefaultWindowHeight = 600;

// void
// createFramebuffer(u32 *fbo, u32 *rbo, u32 *texture, i32 width, i32 height) {
//     // TODO(Jovan): Tidy up
//     glGenFramebuffers(1, fbo);
//     glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

//     glGenTextures(1, texture);
//     glBindTexture(GL_TEXTURE_2D, *texture);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, (void*)0);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0);

//     glGenRenderbuffers(1, rbo);
//     glBindRenderbuffer(GL_RENDERBUFFER, *rbo);
//     glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
//     glBindRenderbuffer(GL_RENDERBUFFER, 0);
//     glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *rbo);

//     // TODO(Jovan): Check for concrete errors
//     if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
//         std::cerr << "[Err] Framebuffer not complete" << std::endl;
//     }

//     glBindFramebuffer(GL_FRAMEBUFFER, 0);
// }

static void
errorCallback(int error, const char* description) {
    std::cerr << "[Err] GLFW: " << description << std::endl;
}

static void
framebufferSizeCallback(GLFWwindow *window, i32 width, i32 height) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
    CurrState->mWindow.mSize = v2(width, height);
}

static void
processButtonState(ButtonState *newState, bool isDown) {
    assert(newState->mEndedDown != isDown);
    newState->mEndedDown = isDown;
    ++newState->mHalfTransitionCount;
}

static void
keyCallback(GLFWwindow *window, i32 key, i32 scode, i32 action, i32 mods) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
    KeyboardController *KC = &CurrState->GetNewInput().mKeyboard;

    if(action == GLFW_PRESS || action == GLFW_RELEASE) {
        bool IsDown = action == GLFW_PRESS;
        switch(key) {
            case GLFW_KEY_SPACE: {
                processButtonState(&KC->mChangeModel, IsDown);
            } break;
            
            case GLFW_KEY_ESCAPE: {
                processButtonState(&KC->mQuit, IsDown);
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            } break;
        }
    }
}

static void
cursorPosCallback(GLFWwindow *window, r64 xNew, r64 yNew) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
    v2 OldPos = CurrState->GetOldInput().mMouse.mCursorPos;
    ImVec2 NewPos = ImGui::GetIO().MousePos;
    MouseController *MC = &CurrState->GetNewInput().mMouse;
    MC->mCursorPos = v2(NewPos.x, NewPos.y);
    MC->mCursorDiff = v2(OldPos.X - NewPos.x, NewPos.y - OldPos.Y);
}

static void
mouseButtonCallback(GLFWwindow *window, i32 button, i32 action, i32 mods) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
    Input *IN = &CurrState->GetNewInput();

    if(action == GLFW_PRESS || action == GLFW_RELEASE) {
        bool IsDown = action == GLFW_PRESS;
        switch(button) {
            case GLFW_MOUSE_BUTTON_LEFT: {
            if(CurrState->GetOldInput().mFirstMouse) {
                IN->mFirstMouse = false;
            }
                processButtonState(&IN->mMouse.mLeft, IsDown);
            } break;
        }
    }
}

static void
scrollCallback(GLFWwindow *window, r64 xoffset, r64 yoffset) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
    CurrState->GetNewInput().mMouse.mScrollOffset = yoffset;
}

static inline r64
currentTimeInSeconds() {
    return glfwGetTime();
}

static inline r64
currentTimeInMillis() {
    return currentTimeInSeconds() * 1000.0;
}

i32
main() {
    if(!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return -1;
    }

    glfwSetErrorCallback(errorCallback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow *GLFWWindow = glfwCreateWindow(DefaultWindowWidth, DefaultWindowHeight, "Frag!", 0, 0);
    if(!GLFWWindow) {
        std::cerr << "[Err] GLFW: Failed creating window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(GLFWWindow, framebufferSizeCallback);
    glfwSetKeyCallback(GLFWWindow, keyCallback);
    glfwSetMouseButtonCallback(GLFWWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(GLFWWindow, cursorPosCallback);
    glfwSetScrollCallback(GLFWWindow, scrollCallback);

    glfwMakeContextCurrent(GLFWWindow);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE(Jovan): Camera init
    Camera OrbitalCamera(45.0f, 2.0f);
    State CurrState(&OrbitalCamera);
    FramebufferGL *Framebuffer = &CurrState.mFramebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer->mId);
    glfwSetWindowUserPointer(GLFWWindow, &CurrState);

    // NOTE(Jovan): Init imgui
    InitUI(GLFWWindow);

    ShaderProgram Phong("../shaders/basic.vert", "../shaders/basic.frag");
    ShaderProgram RiggedPhong("../shaders/rigged.vert", "../shaders/rigged.frag");
    ShaderProgram Debug("../shaders/debug.vert", "../shaders/debug.frag");
    
    v3 ModelPosition = v3(0.0f, 4.0f, -8.0f);
    v3 ModelRotation = v3(0.0f, 0.0f, 0.0f);
    v3 ModelScale    = v3(1.0f);

    u32 ModelVAO;
    std::vector<v2> TexCoords;
    std::vector<v3> Vertices;
    std::vector<v3> Normals;
    std::vector<u32> Indices;
    std::vector<Texture> ModelTextures;

    CurrState.mProjection = Perspective(CurrState.mCamera->mFOV, Framebuffer->mSize.X / (r32) Framebuffer->mSize.Y, 0.1f, 100.0f);
    m44 View(1.0f);
    View = LookAt(CurrState.mCamera->mPosition, CurrState.mCamera->mTarget, CurrState.mCamera->mUp);

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

    r32 StartTimeMillis = currentTimeInMillis();
    r32 EndTimeMillis = currentTimeInMillis();
    r32 BeginTimeMillis = currentTimeInMillis();
    r32 RunningTimeSec = currentTimeInSeconds();
    CurrState.mDT = EndTimeMillis - StartTimeMillis;

    ImGuiWindowFlags MainWindowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoFocusOnAppearing;
    MainWindow Main("Main", MainWindowFlags);
    SceneWindow Scene("Scene", ImGuiWindowFlags_AlwaysAutoResize);
    ModelWindow ModelWindow("Model", ImGuiWindowFlags_AlwaysAutoResize);

    glEnable(GL_TEXTURE_2D);
    GLbitfield ClearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    v4 ClearColor(0x34 / (r32) 255, 0x49 / (r32) 255, 0x5e / (r32) 255, 1.0f);
    while(!glfwWindowShouldClose(GLFWWindow)) {
        StartTimeMillis = currentTimeInMillis();
        RunningTimeSec = (currentTimeInMillis() - BeginTimeMillis) / 1000.0f;
        CurrState.BeginFrame();
        glfwPollEvents();
        
        glUseProgram(Phong.mId);
        if(Scene.mHasResized) {
            CurrState.mProjection = Perspective(CurrState.mCamera->mFOV, Framebuffer->mSize.X / (r32) Framebuffer->mSize.Y, 0.1f, 100.0f);
            Scene.mHasResized = false;
        }

        Phong.SetUniform4m("uProjection", CurrState.mProjection);
        Phong.SetUniform3f("uViewPos", CurrState.mCamera->mPosition);

        View.LoadIdentity();
        View = LookAt(CurrState.mCamera->mPosition, CurrState.mCamera->mTarget, CurrState.mCamera->mUp);
        Phong.SetUniform4m("uView", View);

        FramebufferGL::Bind(Framebuffer->mId, ClearColor, ClearMask, Framebuffer->mSize);

        glUseProgram(RiggedPhong.mId);
        RiggedPhong.SetUniform4m("uProjection", CurrState.mProjection);
        RiggedPhong.SetUniform3f("uViewPos", CurrState.mCamera->mPosition);
        RiggedPhong.SetUniform4m("uView", View);
        RiggedPhong.SetUniform1i("uDisplayBoneIdx", 0);

        // NOTE(Jovan): Render model
        UpdateAndRender(&CurrState);
        std::vector<m44> BoneTransforms;
        CurrState.mCurrModel->CalculateJointTransforms(BoneTransforms, RunningTimeSec);
        for(u32 i = 0; i < BoneTransforms.size(); ++i) {
            RiggedPhong.SetUniform4m("uBones[" + std::to_string(i) + "]", BoneTransforms[i]);
        }

        m44 Model(1.0f);
        Model
            .Translate(ModelPosition)
            .Rotate(quat(v3(1.0f, 0.0f, 0.0f), ModelRotation.X))
            .Rotate(quat(v3(0.0f, 1.0f, 0.0f), ModelRotation.Y))
            .Rotate(quat(v3(0.0f, 0.0f, 1.0f), ModelRotation.Z))
            .Scale(ModelScale);
        RiggedPhong.SetUniform4m("uModel", Model);
        CurrState.mCurrModel->Render(RiggedPhong);

        FramebufferGL::Bind(0, ClearColor, ClearMask, CurrState.mWindow.mSize);
        glUseProgram(0);

        NewFrameUI();

        Main.Render(&CurrState, CurrState.mWindow.mSize.X, CurrState.mWindow.mSize.Y);
        Scene.Render(&CurrState);
        ModelWindow.Render(&CurrState, CurrState.mCurrModel->mFilePath, &ModelPosition[0], &ModelRotation[0] ,&ModelScale[0], CurrState.mCurrModel->mVerticesCount);

        ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Position: %.2f, %.2f, %.2f", CurrState.mCamera->mPosition.X, CurrState.mCamera->mPosition.Y, CurrState.mCamera->mPosition.Z);
        ImGui::Text("Pitch: %.2f", CurrState.mCamera->mPitch * 180.0f / PI);
        ImGui::Text("Yaw: %.2f", CurrState.mCamera->mYaw * 180.0f / PI);
        ImGui::End();

        RenderUI();

        CurrState.EndFrame();
        EndTimeMillis = currentTimeInMillis();
        CurrState.mDT = EndTimeMillis - StartTimeMillis;

        glfwSwapBuffers(GLFWWindow);
    }

    DisposeUI();
    glfwDestroyWindow(GLFWWindow);
    glfwTerminate();
    return 0;
}
