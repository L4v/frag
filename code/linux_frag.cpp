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
#include "util.hpp"

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

static void
_ErrorCallback(int error, const char* description) {
    std::cerr << "[Err] GLFW: " << description << std::endl;
}

static void
_FramebufferSizeCallback(GLFWwindow *window, i32 width, i32 height) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
    CurrState->mWindow->mSize = v2(width, height);
}

static void
_KeyCallback(GLFWwindow *window, i32 key, i32 scode, i32 action, i32 mods) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
    KeyboardController *KC = &CurrState->mInput->mKeyboard;

    if(action == GLFW_PRESS || action == GLFW_RELEASE) {
        bool IsDown = action == GLFW_PRESS;
        switch(key) {
            case GLFW_KEY_SPACE: {
                KC->mChangeModel = IsDown;
            } break;
            
            case GLFW_KEY_ESCAPE: {
                KC->mQuit = IsDown;
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            } break;
        }
    }

    if(key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
        CurrState->mShowBones = !CurrState->mShowBones;
    }
}

static void
_CursorPosCallback(GLFWwindow *window, r64 xNew, r64 yNew) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
}

static void
_MouseButtonCallback(GLFWwindow *window, i32 button, i32 action, i32 mods) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        CurrState->mLeftMouse = true;
    }
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        CurrState->mLeftMouse = false;
    }
}

static void
_ScrollCallback(GLFWwindow *window, r64 xoffset, r64 yoffset) {
    State *CurrState = (State*)glfwGetWindowUserPointer(window);
    if(CurrState->mWindow->mSceneWindowFocused) {
        CurrState->mCamera->Zoom(yoffset, CurrState->mDT);
    }
}

static inline r64
_CurrentTimeInSeconds() {
    return glfwGetTime();
}

static inline r64
_CurrentTimeInMillis() {
    return _CurrentTimeInSeconds() * 1000.0;
}

i32
main() {
    Window FragWindow(800, 600);

    if(!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return -1;
    }
    glfwSetErrorCallback(_ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow *GLFWWindow = glfwCreateWindow(FragWindow.mSize.X, FragWindow.mSize.Y, "Frag!", 0, 0);
    if(!GLFWWindow) {
        std::cerr << "[Err] GLFW: Failed creating window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(GLFWWindow, _FramebufferSizeCallback);
    glfwSetKeyCallback(GLFWWindow, _KeyCallback);
    glfwSetMouseButtonCallback(GLFWWindow, _MouseButtonCallback);
    glfwSetCursorPosCallback(GLFWWindow, _CursorPosCallback);
    glfwSetScrollCallback(GLFWWindow, _ScrollCallback);

    glfwMakeContextCurrent(GLFWWindow);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE(Jovan): Init imgui
    InitUI(GLFWWindow);

    ShaderProgram Phong("../shaders/basic.vert", "../shaders/basic.frag");
    ShaderProgram RiggedPhong("../shaders/rigged.vert", "../shaders/rigged.frag");
    ShaderProgram Debug("../shaders/debug.vert", "../shaders/debug.frag");

    GLTFModel BonesModel("../res/backleg_bones.gltf");
    GLTFModel MusclesModel("../res/backleg_muscles.glb");
    
    v3 ModelPosition = v3(0.0f, 4.0f, -8.0f);
    v3 ModelRotation = v3(0.0f, 0.0f, 0.0f);
    v3 ModelScale    = v3(1.0f);

    u32 ModelVAO;
    std::vector<v2> TexCoords;
    std::vector<v3> Vertices;
    std::vector<v3> Normals;
    std::vector<u32> Indices;
    std::vector<Texture> ModelTextures;

    // NOTE(Jovan): Camera init
    Camera OrbitalCamera(45.0f, 2.0f);
    Input FragInput;
    State CurrState(&FragWindow, &FragInput, &OrbitalCamera);
    glfwSetWindowUserPointer(GLFWWindow, &CurrState);

    CurrState.mProjection = Perspective(CurrState.mCamera->mFOV, CurrState.mFramebufferSize.X / (r32) CurrState.mFramebufferSize.Y, 0.1f, 100.0f);
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

    u32 FBO, RBO;
    _CreateFramebuffer(&FBO, &RBO, &CurrState.mFBOTexture, CurrState.mFramebufferSize.X, CurrState.mFramebufferSize.Y);

    r32 StartTimeMillis = _CurrentTimeInMillis();
    r32 EndTimeMillis = _CurrentTimeInMillis();
    r32 BeginTimeMillis = _CurrentTimeInMillis();
    r32 RunningTimeSec = _CurrentTimeInSeconds();
    CurrState.mDT = EndTimeMillis - StartTimeMillis;

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

    Input FragInput2;
    Input *NewInput = &FragInput;
    Input *OldInput = &FragInput2;
    *NewInput = (Input){0};
    *OldInput = (Input){0};
    while(!glfwWindowShouldClose(GLFWWindow)) {
        CurrState.mInput = NewInput;
        for(u32 i = 0; i < ArrayCount(NewInput->mKeyboard.mButtons); ++i) {
            NewInput->mKeyboard.mButtons[i] = OldInput->mKeyboard.mButtons[i];
        }
        glfwPollEvents();
        if(FragInput.mKeyboard.mChangeModel) {
            CurrState.mShowBones = !CurrState.mShowBones;
        }

        CurrState.mCurrModel = CurrState.mShowBones
            ? &BonesModel
            : &MusclesModel;

        StartTimeMillis = _CurrentTimeInMillis();
        RunningTimeSec = (_CurrentTimeInMillis() - BeginTimeMillis) / 1000.0f;
        
        glUseProgram(Phong.mId);
        if(Scene.mHasResized) {
            OldFBO = FBO;
            OldFBOTexture = CurrState.mFBOTexture;
            OldRBO = RBO;

            _CreateFramebuffer(&FBO, &RBO, &CurrState.mFBOTexture, CurrState.mFramebufferSize.X, CurrState.mFramebufferSize.Y);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            CurrState.mProjection = Perspective(CurrState.mCamera->mFOV, CurrState.mFramebufferSize.X / (r32) CurrState.mFramebufferSize.Y, 0.1f, 100.0f);

            glDeleteFramebuffers(1, &OldFBO);
            glDeleteRenderbuffers(1, &OldRBO);
            glDeleteTextures(1, &OldFBOTexture);
            Scene.mHasResized = false;
        }

        Phong.SetUniform4m("uProjection", CurrState.mProjection);
        Phong.SetUniform3f("uViewPos", CurrState.mCamera->mPosition);

        View.LoadIdentity();
        View = LookAt(CurrState.mCamera->mPosition, CurrState.mCamera->mTarget, CurrState.mCamera->mUp);
        Phong.SetUniform4m("uView", View);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0x34 / (r32) 255, 0x49 / (r32) 255, 0x5e / (r32) 255, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, CurrState.mFramebufferSize.X, CurrState.mFramebufferSize.Y);


        glUseProgram(RiggedPhong.mId);
        RiggedPhong.SetUniform4m("uProjection", CurrState.mProjection);
        RiggedPhong.SetUniform3f("uViewPos", CurrState.mCamera->mPosition);
        RiggedPhong.SetUniform4m("uView", View);
        RiggedPhong.SetUniform1i("uDisplayBoneIdx", 0);

        // NOTE(Jovan): Render model
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

        glUseProgram(Phong.mId);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, FragWindow.mSize.X, FragWindow.mSize.Y);
        glUseProgram(0);

        NewFrameUI();

        Main.Render(&CurrState, FragWindow.mSize.X, FragWindow.mSize.Y);
        Scene.Render(&CurrState);
        ModelWindow.Render(CurrState.mCurrModel->mFilePath, &ModelPosition[0], &ModelRotation[0] ,&ModelScale[0], CurrState.mCurrModel->mVerticesCount);

        ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Position: %.2f, %.2f, %.2f", CurrState.mCamera->mPosition.X, CurrState.mCamera->mPosition.Y, CurrState.mCamera->mPosition.Z);
        ImGui::Text("Pitch: %.2f", CurrState.mCamera->mPitch * 180.0f / PI);
        ImGui::Text("Yaw: %.2f", CurrState.mCamera->mYaw * 180.0f / PI);
        ImGui::End();

        RenderUI();

        EndTimeMillis = _CurrentTimeInMillis();
        CurrState.mDT = EndTimeMillis - StartTimeMillis;

        Input *Tmp = NewInput;
        NewInput = OldInput;
        OldInput = NewInput;

        glfwSwapBuffers(GLFWWindow);
    }

    DisposeUI();
    glfwDestroyWindow(GLFWWindow);
    glfwTerminate();
    return 0;
}
