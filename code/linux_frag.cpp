#include <iostream>
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>

#include "ui.hpp"
#include "types.hpp"
#include "shader.hpp"
#include "model.hpp"

internal i32 _WindowWidth = 800;
internal i32 _WindowHeight = 600;

struct Light {
    v3 Position;
    r32       Size;

    r32       Kc;
    r32       Kl;
    r32       Kq;

    v3 Ambient;
    v3 Diffuse;
    v3 Specular;
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

void
RenderCube() {
    static bool IsLoaded = false;
    static u32 VAO;
    static u32 Buffers[BUFFER_COUNT] = { 0 };
    static u32 IndexCount = 0;
    if (!IsLoaded) {
        std::cout << "Loading cube" << std::endl;

        std::vector<v3> Vertices = {
            v3(-0.5f, -0.5f, -0.5f), 
            v3( 0.5f, -0.5f, -0.5f), 
            v3(-0.5f,  0.5f, -0.5f), 
            v3( 0.5f,  0.5f, -0.5f), 
            v3(-0.5f, -0.5f,  0.5f), 
            v3( 0.5f, -0.5f,  0.5f), 
            v3(-0.5f,  0.5f,  0.5f), 
            v3( 0.5f,  0.5f,  0.5f), 
            v3(-0.5f,  0.5f,  0.5f), 
            v3(-0.5f,  0.5f, -0.5f), 
            v3(-0.5f, -0.5f,  0.5f), 
            v3(-0.5f, -0.5f, -0.5f), 
            v3(0.5f,  0.5f,  0.5f),  
            v3(0.5f,  0.5f, -0.5f),  
            v3(0.5f, -0.5f,  0.5f),  
            v3(0.5f, -0.5f, -0.5f),  
            v3(-0.5f, -0.5f, -0.5f), 
            v3( 0.5f, -0.5f, -0.5f), 
            v3(-0.5f, -0.5f,  0.5f), 
            v3( 0.5f, -0.5f,  0.5f), 
            v3(-0.5f,  0.5f, -0.5f), 
            v3( 0.5f,  0.5f, -0.5f), 
            v3(-0.5f,  0.5f,  0.5f), 
            v3( 0.5f,  0.5f,  0.5f)
        };

        std::vector<v3> Normals = {
            v3(0.0f, 0.0f, 1.0f),  
            v3(0.0f, 0.0f, 1.0f),  
            v3(0.0f, 0.0f, 1.0f),  
            v3(0.0f, 0.0f, 1.0f),  
            v3(0.0f, 0.0f, -1.0f), 
            v3(0.0f, 0.0f, -1.0f), 
            v3(0.0f, 0.0f, -1.0f), 
            v3(0.0f, 0.0f, -1.0f), 
            v3(-1.0f, 0.0f, 0.0f), 
            v3(-1.0f, 0.0f, 0.0f), 
            v3(-1.0f, 0.0f, 0.0f), 
            v3(-1.0f, 0.0f, 0.0f), 
            v3(1.0f, 0.0f, 0.0f),  
            v3(1.0f, 0.0f, 0.0f),  
            v3(1.0f, 0.0f, 0.0f),  
            v3(1.0f, 0.0f, 0.0f),  
            v3(0.0f, -1.0f, 0.0f), 
            v3(0.0f, -1.0f, 0.0f), 
            v3(0.0f, -1.0f, 0.0f), 
            v3(0.0f, -1.0f, 0.0f), 
            v3(0.0f, 1.0f, 0.0f),  
            v3(0.0f, 1.0f, 0.0f),  
            v3(0.0f, 1.0f, 0.0f),  
            v3(0.0f, 1.0f, 0.0f)   
        };

        u32 Indices[] = {
            // NEAR
            0, 1, 2,
            2, 1, 3,

            // FAR
            4, 5, 6,
            6, 5, 7,

            // LEFT
            8, 9, 10,
            10, 9, 11,

            // RIGHT
            12, 13, 14,
            14, 13, 15,

            // BOTTOM
            16, 17, 18,
            18, 17, 19,

            // TOP
            20, 21, 22,
            22, 21, 23
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(BUFFER_COUNT, Buffers);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, Buffers[POS_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices[0]) * Vertices.size() , &Vertices[0], GL_STATIC_DRAW);

        glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(r32), (void*)(0));
        glEnableVertexAttribArray(POSITION_LOCATION);

        glBindBuffer(GL_ARRAY_BUFFER, Buffers[NORM_VB]);
        glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(r32), (void*)(0));
        glEnableVertexAttribArray(NORMAL_LOCATION);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[INDEX_BUFFER]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        IndexCount = ArrayCount(Indices);
        IsLoaded = true;
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[INDEX_BUFFER]);
    glDrawElementsBaseVertex(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, (void*)0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void
RenderModel(Model &model, ShaderProgram &program, r32 runningTime) {
    model.mModel.LoadIdentity();
    model.mModel
        .Translate(model.mPosition)
        .Scale(model.mScale);
    // model.mModel = glm::rotate(model.mModel, glm::radians(model.mRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    // model.mModel = glm::rotate(model.mModel, glm::radians(model.mRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    // model.mModel = glm::rotate(model.mModel, glm::radians(model.mRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    program.SetUniform4m("uModel", model.mModel);

    //std::vector<glm::mat4> Transforms;
    //model.BoneTransform(runningTime, Transforms);
    //program.SetUniform4m("uBones", Transforms, Transforms.size());

    glBindVertexArray(model.mVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.mBuffers[INDEX_BUFFER]);
    for(u32 MeshIdx = 0; MeshIdx < model.mMeshes.size(); ++MeshIdx) {
        Mesh &Mesh = model.mMeshes[MeshIdx];

        // for(u32 TexIdx = 0; TexIdx < Mesh.mTextures.size(); ++TexIdx) {
        //     Texture &Tex = Mesh.mTextures[TexIdx];
        //     glActiveTexture(GL_TEXTURE0 + TexIdx);
        //     // TODO(Jovan): Multiple same-type texture support
        //     if(Tex.mType == Texture::DIFFUSE) {
        //         program.SetUniform1i("uDiffuse", TexIdx);
        //     } else if (Tex.mType == Texture::SPECULAR) {
        //         program.SetUniform1i("uSpecular", TexIdx);
        //     }
        //     glBindTexture(GL_TEXTURE_2D, Tex.mId);
        // }

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

    Model Dragon("../res/models/amongus.obj");
    if(!Dragon.Load()) {
        std::cerr << "[err] failed to load amongus.obj" << std::endl;
    }
    Dragon.mPosition = v3(0.0f);
    // Dragon.mRotation = v3(0.0f, 0.0f, 0.0f);
    Dragon.mScale    = v3(0.008f);

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
    Phong.SetUniform3f("uPointLights[0].Ambient", PointLight.Ambient);
    Phong.SetUniform3f("uPointLights[0].Diffuse", PointLight.Diffuse);
    Phong.SetUniform3f("uPointLights[0].Specular", PointLight.Specular);
    Phong.SetUniform3f("uPointLights[0].Position", PointLight.Position);
    Phong.SetUniform1f("uPointLights[0].Kc", PointLight.Kc);
    Phong.SetUniform1f("uPointLights[0].Kl", PointLight.Kl);
    Phong.SetUniform1f("uPointLights[0].Kq", PointLight.Kq);

    u32 FBO, RBO;
    _CreateFramebuffer(&FBO, &RBO, &State.mFBOTexture, State.mFramebufferSize.X, State.mFramebufferSize.Y);

    r32 StartTime = glfwGetTime();
    r32 EndTime = glfwGetTime();
    r32 BeginTime = glfwGetTime();
    r32 RunningTime = glfwGetTime();
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
        RunningTime = glfwGetTime() - BeginTime;
        
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
        // NOTE(Jovan): Render model
        // glUseProgram(RiggedPhong.mId);
        // RiggedPhong.SetUniform4m("uProjection", State.mProjection);
        // RiggedPhong.SetUniform4m("uView", View);
        // Phong.SetUniform4m("uModel", m44(1.0f));
        // RenderCube();
        RenderModel(Dragon, Phong, RunningTime);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, _WindowWidth, _WindowHeight);
        glUseProgram(0);

        NewFrameUI();

        Main.Render(&State, _WindowWidth, _WindowHeight);
        Scene.Render(&State);
        ModelWindow.Render(Dragon.mFilepath, &Dragon.mPosition[0], &Dragon.mScale[0], Dragon.mNumVertices);

        ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Position: %.2f, %.2f, %.2f", State.mCamera->mPosition.X, State.mCamera->mPosition.Y, State.mCamera->mPosition.Z);
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
