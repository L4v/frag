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
#include <algorithm>
#include <Magick++.h>

#include "include/imgui/imgui.h"
#include "include/imgui/imgui_impl_glfw.h"
#include "include/imgui/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define global static
#define internal static

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

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

#include "shader.cpp"
#include "model.cpp"

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

class Camera {
public:
    enum ECameraMoveDirection {
        MOVE_LEFT  = 0,
        MOVE_RIGHT,
        MOVE_FWD,
        MOVE_BWD,
        MOVE_UP,
        MOVE_DOWN
    };

    r32       mFOV;
    r32       mPitch;
    r32       mYaw;
    r32       mMoveSpeed;
    r32       mRotateSpeed;

    glm::vec3 mWorldUp;
    glm::vec3 mPosition;
    glm::vec3 mFront;
    glm::vec3 mUp;
    glm::vec3 mRight;

    Camera(glm::vec3 position, glm::vec3 worldUp, r32 fov, r32 pitch, r32 yaw, r32 moveSpeed, r32 rotateSpeed) {
        mPosition = position;
        mWorldUp = worldUp;
        mFOV   = fov;
        mPitch = pitch;
        mYaw   = yaw;
        mMoveSpeed = moveSpeed;
        mRotateSpeed = rotateSpeed;
        _UpdateCameraVectors();
    }

    void
    Rotate(r32 dx, r32 dy, r32 dt) {
        dx *= mRotateSpeed;
        dy *= mRotateSpeed;

        mYaw += dx;
        mPitch += dy;
        if(mPitch > 89.0f) {
            mPitch = 89.0f;
        }
        if(mPitch < -89.0f) {
            mPitch = -89.0f;
        }

        _UpdateCameraVectors();
    }

    void
    Move(ECameraMoveDirection direction, r32 dt) {
        r32 Velocity = mMoveSpeed * dt;

        if(direction == MOVE_FWD) {
            mPosition += Velocity * mFront;
        }

        if(direction == MOVE_BWD) {
            mPosition -= Velocity * mFront;
        }

        if(direction == MOVE_LEFT) {
            mPosition -= Velocity * mRight;
        }

        if(direction == MOVE_RIGHT) {
            mPosition += Velocity * mRight;
        }
    }

private:
    void
    _UpdateCameraVectors() {
        mFront.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        mFront.y = sin(glm::radians(mPitch));
        mFront.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
        mFront = glm::normalize(mFront);
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
    State->mProjection = glm::perspective(glm::radians(45.0f), _WindowWidth / (r32) _WindowHeight, 0.1f, 100.0f);

}

internal void
_KeyCallback(GLFWwindow *window, i32 key, i32 scode, i32 action, i32 mods) {
    EngineState *State = (EngineState*)glfwGetWindowUserPointer(window);

    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if(key == GLFW_KEY_A && action != GLFW_RELEASE) {
        State->mCamera->Move(Camera::MOVE_LEFT, State->mDT);
    }
    if(key == GLFW_KEY_D && action != GLFW_RELEASE) {
        State->mCamera->Move(Camera::MOVE_RIGHT, State->mDT);
    }
    if(key == GLFW_KEY_W && action != GLFW_RELEASE) {
        State->mCamera->Move(Camera::MOVE_FWD, State->mDT);
    }
    if(key == GLFW_KEY_S && action != GLFW_RELEASE) {
        State->mCamera->Move(Camera::MOVE_BWD, State->mDT);
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
    r32 dx = State->mCursorPos.x - xNew;
    r32 dy = yNew - State->mCursorPos.y;

    if(State->mLeftMouse && !ImGui::GetIO().WantCaptureMouse) {
        State->mCamera->Rotate(dx, dy, State->mDT);
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

    glfwMakeContextCurrent(Window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE(Jovan): Init imgui
    ImGui::CreateContext();
    //ImGuiIO& IO = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(Window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ShaderProgram Shader("../shaders/basic.vert", "../shaders/basic.frag");

    Model Amongus("../res/models/amongus.obj");
    if(!Amongus.Load()) {
        std::cerr << "[Err] Failed to load amongus.obj" << std::endl;
    }


    // NOTE(Jovan): Camera init
    Camera FPSCamera(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 45.0f, 0.0f, -90.0f, 5.0f, 0.03f);
    EngineState State(&FPSCamera);
    glfwSetWindowUserPointer(Window, &State);

    State.mProjection = glm::perspective(glm::radians(45.0f), _WindowWidth / (r32) _WindowHeight, 0.1f, 100.0f);
    glm::mat4 View = glm::mat4(1.0f);
    View = glm::translate(View, glm::vec3(0.0f, 0.0f, -3.0f));
    View = glm::lookAt(State.mCamera->mPosition, State.mCamera->mPosition + State.mCamera->mFront, State.mCamera->mUp);
    glm::mat4 Model = glm::mat4(1.0f);

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

        ImGui::Text("Hellooooo");

        StartTime = glfwGetTime();
        glfwGetFramebufferSize(Window, &_WindowWidth, &_WindowHeight);
        r32 AspectRatio = _WindowWidth / (r32) _WindowHeight;
        glViewport(0, 0, _WindowWidth, _WindowHeight);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(Shader.mId);

        Shader.SetUniform4m("uProjection", State.mProjection);
        Shader.SetUniform3f("uViewPos", State.mCamera->mPosition);

        View = glm::mat4(1.0f);
        View = glm::translate(View, glm::vec3(0.0f, 0.0f, -3.0f));
        View = glm::lookAt(State.mCamera->mPosition, State.mCamera->mPosition + State.mCamera->mFront, State.mCamera->mUp);
        Shader.SetUniform4m("uView", View);

        Model = glm::mat4(1.0f);
        Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, -3.0f));
        Model = glm::scale(Model, glm::vec3(0.001f));
        Shader.SetUniform4m("uModel", Model);

        // NOTE(Jovan): Render model
        Amongus.Render(Shader);
        std::cout << "Camera pos: " << State.mCamera->mPosition.x << " " << State.mCamera->mPosition.z << std::endl;
        std::cout << "Camera up: " << State.mCamera->mUp.x << " " << State.mCamera->mUp.y << std::endl;
        std::cout << "Camera right: " << State.mCamera->mRight.x << " " << State.mCamera->mRight.z << std::endl;
       
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
