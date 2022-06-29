#include <cassert>
#include <iostream>
#include "framebuffer_gl.hpp"
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>
#include <algorithm>


#include "include/imgui/imgui.h"
#include "include/imgui/imgui_internal.h"
#include "include/imgui/imgui_impl_glfw.h"
#include "include/imgui/imgui_impl_opengl3.h"

#include "frag.hpp"
#include "types.hpp"
#include "shader.hpp"
#include "model.hpp"

static const i32 DefaultWindowWidth = 800;
static const i32 DefaultWindowHeight = 600;

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
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(GLFWWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    Shader Phong("../shaders/basic.vert", "../shaders/basic.frag");
    Shader RiggedPhong("../shaders/rigged.vert", "../shaders/rigged.frag");
    Shader Debug("../shaders/debug.vert", "../shaders/debug.frag");
    
    v3 ModelPosition = v3(0.0f, 4.0f, -8.0f);
    v3 ModelRotation = v3(0.0f, 0.0f, 0.0f);
    v3 ModelScale    = v3(1.0f);

    u32 ModelVAO;
    std::vector<v2> TexCoords;
    std::vector<v3> Vertices;
    std::vector<v3> Normals;
    std::vector<u32> Indices;
    std::vector<Texture> ModelTextures;

    CurrState.mProjection = Perspective(OrbitalCamera.mFOV, Framebuffer->mSize.X / (r32) Framebuffer->mSize.Y, 0.1f, 100.0f);
    m44 View(1.0f);
    View = LookAt(OrbitalCamera.mPosition, OrbitalCamera.mTarget, OrbitalCamera.mUp);

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
    // r32 RunningTimeSec = currentTimeInSeconds();
    CurrState.mCurrentTimeInSeconds = currentTimeInSeconds();
    CurrState.mDT = EndTimeMillis - StartTimeMillis;

    ImGuiWindowFlags MainWindowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoFocusOnAppearing;
    bool IsMainWindowInitialized = false;
    const std::string MainWindowName = "Main";
    const std::string SceneWindowName = "Scene";
    const std::string ModelWindowName = "Model";
    i32 CurrentProjection = 0;

    glEnable(GL_TEXTURE_2D);
    GLbitfield ClearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    v4 ClearColor(0x34 / (r32) 255, 0x49 / (r32) 255, 0x5e / (r32) 255, 1.0f);
    while(!glfwWindowShouldClose(GLFWWindow)) {
        StartTimeMillis = currentTimeInMillis();
        CurrState.mCurrentTimeInSeconds = (currentTimeInMillis() - BeginTimeMillis) / 1000.0f;
        CurrState.BeginFrame();
        glfwPollEvents();
        
        glUseProgram(Phong.mId);

        Phong.SetUniform4m("uProjection", CurrState.mProjection);
        Phong.SetUniform3f("uViewPos", OrbitalCamera.mPosition);

        View.LoadIdentity();
        View = LookAt(OrbitalCamera.mPosition, OrbitalCamera.mTarget, OrbitalCamera.mUp);
        Phong.SetUniform4m("uView", View);

        FramebufferGL::Bind(Framebuffer->mId, ClearColor, ClearMask, Framebuffer->mSize);

        glUseProgram(RiggedPhong.mId);
        RiggedPhong.SetUniform4m("uProjection", CurrState.mProjection);
        RiggedPhong.SetUniform3f("uViewPos", OrbitalCamera.mPosition);
        RiggedPhong.SetUniform4m("uView", View);
        RiggedPhong.SetUniform1i("uDisplayBoneIdx", 0);

        // NOTE(Jovan): Render model
        CurrState.mPerspective = CurrentProjection == 0;
        UpdateAndRender(&CurrState);
        std::vector<m44> BoneTransforms;
        GLTFModel *CurrModel = CurrState.mCurrModel;
        CurrModel->CalculateJointTransforms(BoneTransforms, CurrState.mCurrentTimeInSeconds);
        RiggedPhong.SetUniform4m("uBones", BoneTransforms);

        
        CurrModel->mModelTransform
            .LoadIdentity()
            .Translate(ModelPosition)
            .Rotate(quat(v3(1.0f, 0.0f, 0.0f), ModelRotation.X))
            .Rotate(quat(v3(0.0f, 1.0f, 0.0f), ModelRotation.Y))
            .Rotate(quat(v3(0.0f, 0.0f, 1.0f), ModelRotation.Z))
            .Scale(ModelScale);
        CurrModel->Render(RiggedPhong);

        FramebufferGL::Bind(0, ClearColor, ClearMask, CurrState.mWindow.mSize);
        glUseProgram(0);

        // NOTE(Jovan): UI RENDERING ========================================
        // NOTE(Jovan): New frame UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // NOTE(Jovan): Main window
        ImVec2 Size(CurrState.mWindow.mSize.X, CurrState.mWindow.mSize.Y);
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(Size);
        ImGui::Begin(MainWindowName.c_str(), NULL, MainWindowFlags);
        ImVec2 Pos(0.0f, 0.0f);
        ImGuiID DockID = ImGui::GetID("DockSpace");
        ImGui::DockSpace(DockID);

        if (!IsMainWindowInitialized) {
            ImGui::DockBuilderRemoveNode(DockID);
            ImGui::DockBuilderAddNode(DockID, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(DockID, Size);
            ImGuiID LeftID = ImGui::DockBuilderSplitNode(DockID, ImGuiDir_Left, 0.5, NULL, &DockID);
            ImGuiID BottomID = ImGui::DockBuilderSplitNode(LeftID, ImGuiDir_Down, 0.5f, NULL, &LeftID);
            ImGui::DockBuilderDockWindow("Model", LeftID);
            ImGui::DockBuilderDockWindow("Camera", BottomID);
            ImGui::DockBuilderDockWindow("Scene",  DockID);
            ImGui::DockBuilderFinish(DockID);
            IsMainWindowInitialized = true;
        }

        ImGui::End();

        // NOTE(Jovan): Scene window
        ImGui::Begin(SceneWindowName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize); {
            ImGui::BeginChild("SceneRender");
            CurrState.mWindow.mSceneWindowFocused = ImGui::IsWindowFocused();

            ImVec2 WindowSize = ImGui::GetWindowSize();
            if (CurrState.mFramebuffer.mSize.X != WindowSize.x || CurrState.mFramebuffer.mSize.Y != WindowSize.y) {
                CurrState.mFramebuffer.Resize(WindowSize.x, WindowSize.y);
            }

            ImGui::Image((ImTextureID)CurrState.mFramebuffer.mTexture, WindowSize, ImVec2(0, 1), ImVec2(1, 0));
            ImGui::EndChild();
        } ImGui::End();

        // NOTE(Jovan): Model window
        ImGui::Begin(ModelWindowName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Loaded model: %s", CurrModel->mFilePath.c_str());
        ImGui::Checkbox("Show bones", &CurrState.mShowBones);
        ImGui::Spacing();
        ImGui::DragFloat3("Position", ModelPosition.Values, 1e-3f);
        ImGui::DragFloat3("Rotation", ModelRotation.Values, 1e-1f);
        ImGui::DragFloat3("Scale", ModelScale.Values, 1e-3f);
        ImGui::Spacing();
        ImGui::Text("Vertices: %d", CurrModel->mVerticesCount);
        ImGui::Spacing();
        ImGui::SliderFloat("Animation speed", &CurrModel->mActiveAnimation->mSpeed, 1e-2f, 4.0f, "%.2f");
        ImGui::End();

        // NOTE(Jovan): Camera window
        ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::DragFloat3("Position", OrbitalCamera.mPosition.Values, 1e-3f);
        ImGui::Spacing();
        ImGui::DragFloat("FOV", &OrbitalCamera.mFOV, 1.0f, 15.0f, 120.0f, "%.1f");
        ImGui::Spacing();
        const char *ProjectionTypes[] = {"Perspective", "Orthographic"};
        ImGui::Combo("##Projection", &CurrentProjection, ProjectionTypes, 2);
        ImGui::Text("Pitch: %.2f", OrbitalCamera.mPitch * DEG);
        ImGui::Text("Yaw: %.2f", OrbitalCamera.mYaw * DEG);
        ImGui::End();

        // NOTE(Jovan): Render UI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        CurrState.EndFrame();
        EndTimeMillis = currentTimeInMillis();
        CurrState.mDT = EndTimeMillis - StartTimeMillis;

        glfwSwapBuffers(GLFWWindow);
    }

    // NOTE(Jovan): Dispose UI
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(GLFWWindow);
    glfwTerminate();
    return 0;
}
