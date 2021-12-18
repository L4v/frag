#include <iostream>
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>
#include <Magick++.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "frag.hpp"
#include "ui.hpp"
#include "types.hpp"
#include "shader.hpp"
#include "model.hpp"

internal i32 _WindowWidth = 800;
internal i32 _WindowHeight = 600;

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

    ShaderProgram Shader("../shaders/basic.vert", "../shaders/basic.frag");

    Model Amongus("../res/models/amongus.obj");
    if(!Amongus.Load()) {
        std::cerr << "[Err] Failed to load amongus.obj" << std::endl;
    }
    Amongus.mPosition = glm::vec3(0.0f);
    Amongus.mRotation = glm::vec3(0.0f);
    Amongus.mScale    = glm::vec3(1e-3f);

    // NOTE(Jovan): Camera init
    Camera OrbitalCamera(45.0f, 2.0f);
    EngineState State(&OrbitalCamera);
    glfwSetWindowUserPointer(Window, &State);

    State.mProjection = glm::perspective(glm::radians(State.mCamera->mFOV), State.mFramebufferSize.x / (r32) State.mFramebufferSize.y, 0.1f, 100.0f);
    glm::mat4 View = glm::mat4(1.0f);
    View = glm::lookAt(State.mCamera->mPosition, State.mCamera->mTarget, State.mCamera->mUp);

    // NOTE(Jovan): Set texture scale
    glUseProgram(Shader.mId);
    Shader.SetUniform1f("uTexScale", 1.0f);
    Shader.SetUniform4m("uProjection", State.mProjection);
    Shader.SetUniform4m("uView", View);

    Light PointLight;
    PointLight.Ambient = glm::vec3(0.3f);
    PointLight.Diffuse = glm::vec3(0.5f);
    PointLight.Specular = glm::vec3(1.0f);
    PointLight.Kc = 1.0f;
    PointLight.Kl = 0.09f;
    PointLight.Kq = 0.032f;
    PointLight.Position = glm::vec3(0.0f, 1.0f, 2.0f);
    Shader.SetUniform3f("uPointLights[0].Ambient", PointLight.Ambient);
    Shader.SetUniform3f("uPointLights[0].Diffuse", PointLight.Diffuse);
    Shader.SetUniform3f("uPointLights[0].Specular", PointLight.Specular);
    Shader.SetUniform3f("uPointLights[0].Position", PointLight.Position);
    Shader.SetUniform1f("uPointLights[0].Kc", PointLight.Kc);
    Shader.SetUniform1f("uPointLights[0].Kl", PointLight.Kl);
    Shader.SetUniform1f("uPointLights[0].Kq", PointLight.Kq);

    //Shader.SetUniform3f("uMaterial.Ambient", glm::vec3(1.0f, 0.0f, 0.0f));
    //Shader.SetUniform3f("uMaterial.Diffuse", glm::vec3(0.3f, 0.3f, 0.3f));
    //Shader.SetUniform3f("uMaterial.Specular", glm::vec3(0.8f));
    //Shader.SetUniform1f("uMaterial.Shininess", 128.0f);

    u32 FBO, RBO;
    _CreateFramebuffer(&FBO, &RBO, &State.mFBOTexture, State.mFramebufferSize.x, State.mFramebufferSize.y);

    r32 StartTime = glfwGetTime();
    r32 EndTime = glfwGetTime();
    State.mDT = EndTime - StartTime;

    u32 OldFBO;
    u32 OldRBO;
    u32 OldFBOTexture;

    ImGuiWindowFlags MainWindowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoFocusOnAppearing;
    MainWindow Main("Main", MainWindowFlags);
    SceneWindow Scene("Scene", ImGuiWindowFlags_AlwaysAutoResize);
    ModelWindow ModelWindow("Model", ImGuiWindowFlags_AlwaysAutoResize);

    while(!glfwWindowShouldClose(Window)) {

        StartTime = glfwGetTime();
        
        glUseProgram(Shader.mId);
        if(Scene.mHasResized) {
            std::cout << "Resizing" << std::endl;
            OldFBO = FBO;
            OldFBOTexture = State.mFBOTexture;
            OldRBO = RBO;

            _CreateFramebuffer(&FBO, &RBO, &State.mFBOTexture, State.mFramebufferSize.x, State.mFramebufferSize.y);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            State.mProjection = glm::perspective(glm::radians(State.mCamera->mFOV), State.mFramebufferSize.x / (r32) State.mFramebufferSize.y, 0.1f, 100.0f);

            glDeleteFramebuffers(1, &OldFBO);
            glDeleteRenderbuffers(1, &OldRBO);
            glDeleteTextures(1, &OldFBOTexture);
            Scene.mHasResized = false;
        }

        Shader.SetUniform4m("uProjection", State.mProjection);
        Shader.SetUniform3f("uViewPos", State.mCamera->mPosition);

        View = glm::mat4(1.0f);
        View = glm::lookAt(State.mCamera->mPosition, State.mCamera->mTarget, State.mCamera->mUp);
        Shader.SetUniform4m("uView", View);

        Shader.SetUniform4m("uModel", Amongus.mModel);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glClearColor(0x34 / (r32) 255, 0x49 / (r32) 255, 0x5e / (r32) 255, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, State.mFramebufferSize.x, State.mFramebufferSize.y);
        // NOTE(Jovan): Render model
        Amongus.Render(Shader);
       
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, _WindowWidth, _WindowHeight);
        glUseProgram(0);

        NewFrameUI();

        Main.Render(&State, _WindowWidth, _WindowHeight);
        Scene.Render(&State);
        ModelWindow.Render(Amongus.mFilename, &Amongus.mPosition[0], &Amongus.mRotation[0], &Amongus.mScale[0]);

        ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Position: %.2f, %.2f, %.2f", State.mCamera->mPosition.x, State.mCamera->mPosition.y, State.mCamera->mPosition.z);
        ImGui::Text("Pitch: %.2f", State.mCamera->mPitch * 180.0f / PI);
        ImGui::Text("Yaw: %.2f", State.mCamera->mYaw * 180.0f / PI);
        ImGui::End();

        RenderUI();

        EndTime = glfwGetTime();
        State.mDT = EndTime - StartTime;

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    DisposeUI();
    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
