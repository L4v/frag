#include <iostream>
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>
#include <Magick++.h>

#include "include/imgui/imgui.h"
#include "include/imgui/imgui_impl_glfw.h"
#include "include/imgui/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

#include "types.hpp"
#include "shader.hpp"
#include "model.hpp"

internal i32 _WindowWidth = 800;
internal i32 _WindowHeight = 600;
internal bool _FirstMouse = true;

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

// TODO(Jovan): Panning?
class Camera {
public:
    r32         mFOV;
    r32         mPitch;
    r32         mYaw;
    r32         mRotateSpeed;
    r32         mDistance;
    r32         mZoomSpeed;

    glm::vec3 mWorldUp;
    glm::vec3 mPosition;
    glm::vec3 mFront;
    glm::vec3 mUp;
    glm::vec3 mRight;
    glm::vec3 mTarget;

    // NOTE(Jovan): Orbital camera constructor
    Camera(r32 fov, r32 distance, r32 rotateSpeed = 8.0f, r32 zoomSpeed = 2.0f, glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3 target = glm::vec3(0.0f)) {
        mFOV = fov;
        mDistance = distance;
        mRotateSpeed = rotateSpeed;
        mZoomSpeed = zoomSpeed;
        mWorldUp = worldUp;
        mTarget = target;

        mYaw = 0.0f;
        mPitch = 0.0f;
        _UpdateVectors();
    }

    void
    Rotate(r32 dYaw, r32 dPitch, r32 dt) {
        dYaw *= mRotateSpeed * dt;
        dPitch *= mRotateSpeed * dt;

        mYaw -= dYaw;
        mPitch -= dPitch;
        if(mPitch > 89.0f) {
            mPitch = 89.0f;
        }
        if(mPitch < -89.0f) {
            mPitch = -89.0f;
        }
        _UpdateVectors();
    }

    void
    Zoom(r32 dy, r32 dt) {
        dy *= mZoomSpeed * dt;
        mDistance -= dy;
        if (mDistance <= 1.0f) {
            mDistance = 1.0f;
        }

        _UpdateVectors();
    }

private:
    void
    _UpdateVectors() {
        mPosition.x = mDistance * cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        mPosition.y = -mDistance * sin(glm::radians(mPitch));
        mPosition.z = mDistance * sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        mFront = glm::normalize(mTarget - mPosition);
        mRight = glm::normalize(glm::cross(mFront, mWorldUp));
        mUp = glm::normalize(glm::cross(mRight, mFront));
    }
};


struct EngineState {
    Camera    *mCamera;
    glm::mat4 mProjection;
    glm::vec2 mCursorPos;
    r32       mDT;
    bool      mLeftMouse;

    EngineState(Camera *camera) {
        mCamera = camera;
        mProjection = glm::mat4(1.0f);
        mCursorPos = glm::vec2(0.0f, 0.0f);
        mDT = 0.0f;
        mLeftMouse = false;
    }
};

internal void
_ErrorCallback(int error, const char* description) {
    std::cerr << "[Err] GLFW: " << description << std::endl;
}

internal void
_FramebufferSizeCallback(GLFWwindow *window, i32 width, i32 height) {
    EngineState *State = (EngineState*) glfwGetWindowUserPointer(window);
    _WindowWidth = width;
    _WindowHeight = height;
    glViewport(0, 0, width, height);
    State->mProjection = glm::perspective(glm::radians(State->mCamera->mFOV), _WindowWidth / (r32) _WindowHeight, 0.1f, 100.0f);

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
    if(_FirstMouse) {
        State->mCursorPos.x = xNew;
        State->mCursorPos.y = yNew;
        _FirstMouse = false;
    }
    r32 DX = State->mCursorPos.x - xNew;
    r32 DY = yNew - State->mCursorPos.y;
    bool WantCaptureMouse = ImGui::GetIO().WantCaptureMouse;

    if(State->mLeftMouse && !WantCaptureMouse) {
        State->mCamera->Rotate(DX, DY, State->mDT);
    }

    State->mCursorPos.x = xNew;
    State->mCursorPos.y = yNew;
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
    State->mCamera->Zoom(yoffset, State->mDT);
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
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(Window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

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

    State.mProjection = glm::perspective(glm::radians(State.mCamera->mFOV), _WindowWidth / (r32) _WindowHeight, 0.1f, 100.0f);
    glm::mat4 View = glm::mat4(1.0f);
    View = glm::lookAt(State.mCamera->mPosition, State.mCamera->mTarget, State.mCamera->mUp);

    // NOTE(Jovan): Set texture scale
    glUseProgram(Shader.mId);
    Shader.SetUniform1f("uTexScale", 1.0f);
    Shader.SetUniform4m("uProjection", State.mProjection);
    Shader.SetUniform4m("uView", View);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0x34 / (r32) 255, 0x49 / (r32) 255, 0x5e / (r32) 255, 1.0f);

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

    r32 StartTime = glfwGetTime();
    r32 EndTime = glfwGetTime();
    State.mDT = EndTime - StartTime;

    while(!glfwWindowShouldClose(Window)) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiWindowFlags Flags = ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::Begin("Model", NULL, Flags);
        const std::string LoadedModelText = "Loaded model: " + Amongus.mFilename;
        ImGui::Text(LoadedModelText.c_str());
        const std::string CameraFront = "Front: " + std::to_string(State.mCamera->mFront.x) + " " + std::to_string(State.mCamera->mFront.y) + " " + std::to_string(State.mCamera->mFront.z);
        const std::string CameraPos = "Position: " +  std::to_string(State.mCamera->mPosition.x) + " " + std::to_string(State.mCamera->mPosition.y) + " " + std::to_string(State.mCamera->mPosition.z);
        const std::string CameraTarget = "Target: " + std::to_string(State.mCamera->mTarget.x) + " " +  std::to_string(State.mCamera->mTarget.y) + " " +    std::to_string(State.mCamera->mTarget.z);
        const std::string CameraYP = "Yaw: " + std::to_string(State.mCamera->mYaw) + " " +  std::to_string(State.mCamera->mPitch);
        ImGui::Text(CameraFront.c_str());
        ImGui::Text(CameraPos.c_str());
        ImGui::Text(CameraTarget.c_str());
        ImGui::Text(CameraYP.c_str());
        ImGui::Spacing();
        ImGui::DragFloat3("Position", &Amongus.mPosition[0], 1e-3f);
        ImGui::DragFloat3("Rotation", &Amongus.mRotation[0], 1e-1f);
        ImGui::DragFloat3("Scale", &Amongus.mScale[0], 1e-3f);
        
        ImGui::End();

        StartTime = glfwGetTime();
        glfwGetFramebufferSize(Window, &_WindowWidth, &_WindowHeight);
        r32 AspectRatio = _WindowWidth / (r32) _WindowHeight;
        glViewport(0, 0, _WindowWidth, _WindowHeight);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(Shader.mId);

        Shader.SetUniform4m("uProjection", State.mProjection);
        Shader.SetUniform3f("uViewPos", State.mCamera->mPosition);

        View = glm::mat4(1.0f);
        View = glm::lookAt(State.mCamera->mPosition, State.mCamera->mTarget, State.mCamera->mUp);
        Shader.SetUniform4m("uView", View);

        Shader.SetUniform4m("uModel", Amongus.mModel);

        // NOTE(Jovan): Render model
        Amongus.Render(Shader);
       
        glUseProgram(0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        EndTime = glfwGetTime();
        State.mDT = EndTime - StartTime;

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
