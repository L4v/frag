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

global i32 G_WWIDTH = 800;
global i32 G_WHEIGHT = 600;

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

        Position     = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
        Target       = glm::vec3(0.0f, 0.0f, -1.0f);
        Direction    = glm::normalize(Position - Target);
        Front        = glm::vec3(0.0f, 0.0f, -1.0f);
        Right        = glm::normalize(glm::cross(Up, Direction));
        this->Up     = glm::cross(Direction, Right);
    }
};

struct EngineState {
    Camera *mCamera;

    EngineState(Camera *camera) {
        mCamera = camera;
    }
};

internal void
_ErrorCallback(int error, const char* description) {
    std::cerr << "[Err] GLFW: " << description << std::endl;
}

internal void
_KeyCallback(GLFWwindow *window, i32 key, i32 scode, i32 action, i32 mods) {
    EngineState *State = (EngineState*)glfwGetWindowUserPointer(window);

    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if(key == GLFW_KEY_A && action != GLFW_RELEASE) {
        State->mCamera->Position -= State->mCamera->Speed * State->mCamera->Right;
    }
    if(key == GLFW_KEY_D && action != GLFW_RELEASE) {
        State->mCamera->Position += State->mCamera->Speed * State->mCamera->Right;
    }
    if(key == GLFW_KEY_W && action != GLFW_RELEASE) {
        State->mCamera->Position += State->mCamera->Speed * State->mCamera->Front;
    }
    if(key == GLFW_KEY_S && action != GLFW_RELEASE) {
        State->mCamera->Position -= State->mCamera->Speed * State->mCamera->Front;
    }
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

    ShaderProgram Shader("../shaders/basic.vert", "../shaders/basic.frag");

    Model Amongus("../res/models/amongus.obj");
    if(!Amongus.Load()) {
        std::cerr << "[Err] Failed to load amongus.obj" << std::endl;
    }


    // NOTE(Jovan): Camera init
    Camera Camera(45.0f, 0.0f, -90.0f, 0.05f);
    EngineState State(&Camera);
    glfwSetWindowUserPointer(Window, &State);

    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::mat4 View = glm::mat4(1.0f);
    View = glm::translate(View, glm::vec3(0.0f, 0.0f, -3.0f));
    View = glm::lookAt(Camera.Position, Camera.Position + Camera.Front, Camera.Up);
    glm::mat4 Model = glm::mat4(1.0f);

    // NOTE(Jovan): Set texture scale
    glUseProgram(Shader.mId);
    Shader.SetUniform1f("uTexScale", 1.0f);
    Shader.SetUniform4m("uProjection", Projection);
    Shader.SetUniform4m("uView", View);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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


    while(!glfwWindowShouldClose(Window)) {
        glfwGetFramebufferSize(Window, &G_WWIDTH, &G_WHEIGHT);
        r32 AspectRatio = G_WWIDTH / (float) G_WHEIGHT;
        glViewport(0, 0, G_WWIDTH, G_WHEIGHT);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(Shader.mId);

        Shader.SetUniform3f("uViewPos", Camera.Position);

        View = glm::mat4(1.0f);
        View = glm::translate(View, glm::vec3(0.0f, 0.0f, -3.0f));
        View = glm::lookAt(Camera.Position, Camera.Position + Camera.Front, Camera.Up);
        Shader.SetUniform4m("uView", View);

        Model = glm::mat4(1.0f);
        Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, -3.0f));
        Model = glm::scale(Model, glm::vec3(0.001f));
        Shader.SetUniform4m("uModel", Model);

        // NOTE(Jovan): Render model
        Amongus.Render(Shader);
        std::cout << "Camera pos: " << State.mCamera->Position.x << " " << State.mCamera->Position.z << std::endl;
       
        glUseProgram(0);


        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
