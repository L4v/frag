#include "framebuffer_gl.hpp"
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

#include "include/imgui/imgui.h"
#include "include/imgui/imgui_impl_glfw.h"
#include "include/imgui/imgui_impl_opengl3.h"
#include "include/imgui/imgui_internal.h"

#include "frag.hpp"
#include "model.hpp"
#include "shader.hpp"
#include "types.hpp"

static const i32 DefaultWindowWidth = 800;
static const i32 DefaultWindowHeight = 600;

static void errorCallback(int error, const char *description) {
  std::cerr << "[Err] GLFW: " << description << std::endl;
}

static void framebufferSizeCallback(GLFWwindow *window, i32 width, i32 height) {
  State *CurrState = (State *)glfwGetWindowUserPointer(window);
  CurrState->mWindow.mSize = v2(width, height);
}

static void processButtonState(ButtonState *newState, bool isDown) {
  assert(newState->mEndedDown != isDown);
  newState->mEndedDown = isDown;
  ++newState->mHalfTransitionCount;
}

static void keyCallback(GLFWwindow *window, i32 key, i32 scode, i32 action,
                        i32 mods) {
  State *CurrState = (State *)glfwGetWindowUserPointer(window);
  KeyboardController *KC = &CurrState->GetNewInput().mKeyboard;

  if (action == GLFW_PRESS || action == GLFW_RELEASE) {
    bool IsDown = action == GLFW_PRESS;
    switch (key) {
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

static void cursorPosCallback(GLFWwindow *window, r64 xNew, r64 yNew) {
  State *CurrState = (State *)glfwGetWindowUserPointer(window);
  v2 OldPos = CurrState->GetOldInput().mMouse.mCursorPos;
  ImVec2 NewPos = ImGui::GetIO().MousePos;
  MouseController *MC = &CurrState->GetNewInput().mMouse;
  MC->mCursorPos = v2(NewPos.x, NewPos.y);
  MC->mCursorDiff = v2(OldPos.X - NewPos.x, NewPos.y - OldPos.Y);
}

static void mouseButtonCallback(GLFWwindow *window, i32 button, i32 action,
                                i32 mods) {
  State *CurrState = (State *)glfwGetWindowUserPointer(window);
  Input *IN = &CurrState->GetNewInput();

  if (action == GLFW_PRESS || action == GLFW_RELEASE) {
    bool IsDown = action == GLFW_PRESS;
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT: {
      if (CurrState->GetOldInput().mFirstMouse) {
        IN->mFirstMouse = false;
      }
      processButtonState(&IN->mMouse.mLeft, IsDown);
    } break;
    }
  }
}

static void scrollCallback(GLFWwindow *window, r64 xoffset, r64 yoffset) {
  State *CurrState = (State *)glfwGetWindowUserPointer(window);
  CurrState->GetNewInput().mMouse.mScrollOffset = yoffset;
}

static inline r64 currentTimeInSeconds() { return glfwGetTime(); }

static inline r64 currentTimeInMillis() {
  return currentTimeInSeconds() * 1000.0;
}

static Light createWhiteLight(r32 x, r32 y, r32 z) {
  Light pointLight;
  pointLight.Ambient = v3(0.3f);
  pointLight.Diffuse = v3(0.5f);
  pointLight.Specular = v3(1.0f);
  pointLight.Kc = 1.0f;
  pointLight.Kl = 0.09f;
  pointLight.Kq = 0.032f;
  pointLight.Position = v3(x, y, z);
  return pointLight;
}

static GLFWwindow *initGlfwWindow() {
  glfwSetErrorCallback(errorCallback);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  GLFWwindow *glfwWindow =
      glfwCreateWindow(DefaultWindowWidth, DefaultWindowHeight, "Frag!", 0, 0);
  if (!glfwWindow) {
    return NULL;
  }
  glfwSetFramebufferSizeCallback(glfwWindow, framebufferSizeCallback);
  glfwSetKeyCallback(glfwWindow, keyCallback);
  glfwSetMouseButtonCallback(glfwWindow, mouseButtonCallback);
  glfwSetCursorPosCallback(glfwWindow, cursorPosCallback);
  glfwSetScrollCallback(glfwWindow, scrollCallback);

  glfwMakeContextCurrent(glfwWindow);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(1);

  return glfwWindow;
}

struct TimeInfo {
  r64 frameStartMs;
  r64 frameEndMs;
  r64 runtimeStartMs;

  void init() {
    frameStartMs = currentTimeInMillis();
    frameEndMs = currentTimeInMillis();
    runtimeStartMs = currentTimeInMillis();
  }

  r64 getDeltaMs() { return frameEndMs - frameStartMs; }
};

void resizePickingFramebuffer(u32 fbo, u32 rbo, u32 texture, r32 width,
                              r32 height) {
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, (void *)0);

  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
}

i32 main() {
  if (!glfwInit()) {
    std::cerr << "Failed to init GLFW" << std::endl;
    return -1;
  }

  GLFWwindow *glfwWindow = initGlfwWindow();
  if (!glfwWindow) {
    std::cerr << "[Err] GLFW: Failed creating window" << std::endl;
    glfwTerminate();
    return -1;
  }

  // NOTE(Jovan): Camera init
  OrbitalCamera camera(45.0f, 10.0f);
  State currState(&camera);
  FramebufferGL *framebuffer = &currState.mFramebuffer;
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->mId);
  glfwSetWindowUserPointer(glfwWindow, &currState);

  // NOTE(Jovan): Init imgui
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  Shader riggedPhong("../shaders/rigged.vert", "../shaders/rigged.frag");
  Shader pickingShader("../shaders/picking.vert", "../shaders/picking.frag");

  v3 modelPosition = v3(0.0f);
  v3 modelRotation = v3(0.0f);
  v3 modelScale = v3(1.0f);

  currState.mProjection =
      perspective(camera.mFOV, framebuffer->mSize.X / (r32)framebuffer->mSize.Y,
                  0.1f, 100.0f);
  m44 view(1.0f);
  view = LookAt(camera.mPosition, camera.mTarget, camera.mUp);

  Light whiteLight = createWhiteLight(0, 1, 2);

  glUseProgram(riggedPhong.mId);
  riggedPhong.SetUniform1f("uTexScale", 1.0f);
  riggedPhong.SetPointLight(whiteLight, 0);

  TimeInfo timeInfo;
  timeInfo.init();
  currState.mCurrentTimeInSeconds = currentTimeInSeconds();
  currState.mDT = timeInfo.getDeltaMs();

  ImGuiWindowFlags mainWindowFlags =
      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoFocusOnAppearing;
  bool isMainWindowInitialized = false;
  const std::string mainWindowName = "Main";
  const std::string sceneWindowName = "Scene";
  const std::string modelWindowName = "Model";

  v2 sceneCursorPos(0.0f);
  v2 sceneWindowPos(0.0f);

  u32 pickingTexture;
  glGenTextures(1, &pickingTexture);
  glBindTexture(GL_TEXTURE_2D, pickingTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, currState.mWindow.mSize.X,
               currState.mWindow.mSize.Y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               (void *)0);

  u32 pickingDepthBuffer;
  glGenRenderbuffers(1, &pickingDepthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, pickingDepthBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                        currState.mWindow.mSize.X, currState.mWindow.mSize.Y);

  u32 pickingFramebuffer;
  glGenFramebuffers(1, &pickingFramebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, pickingFramebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         pickingTexture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, pickingDepthBuffer);

  u32 pickedId = 0;

  glEnable(GL_TEXTURE_2D);
  GLbitfield clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
  v4 clearColor(0x34 / (r32)255, 0x49 / (r32)255, 0x5e / (r32)255, 1.0f);
  while (!glfwWindowShouldClose(glfwWindow)) {
    timeInfo.frameStartMs = currentTimeInMillis();
    currState.mCurrentTimeInSeconds =
        (currentTimeInMillis() - timeInfo.runtimeStartMs) / 1000.0f;
    currState.BeginFrame();
    glfwPollEvents();

    MouseController mouseController = currState.GetNewInput().mMouse;
    sceneCursorPos = mouseController.mCursorPos - sceneWindowPos;

    UpdateState(&currState);
    std::vector<m44> BoneTransforms;
    Model *currModel = currState.mCurrModel;
    currModel->calculateJointTransforms(BoneTransforms,
                                        currState.mCurrentTimeInSeconds);
    currModel->mModelTransform.LoadIdentity()
        .Translate(modelPosition)
        .Rotate(quat(v3(1.0f, 0.0f, 0.0f), modelRotation.X))
        .Rotate(quat(v3(0.0f, 1.0f, 0.0f), modelRotation.Y))
        .Rotate(quat(v3(0.0f, 0.0f, 1.0f), modelRotation.Z))
        .Scale(modelScale);

    view.LoadIdentity();
    view = LookAt(camera.mPosition, camera.mTarget, camera.mUp);

    FramebufferGL::Bind(pickingFramebuffer, v4(0.0f, 0.0f, 0.0f, 1.0f),
                        clearMask, framebuffer->mSize);

    // PICKING PHASE ===============
    glUseProgram(pickingShader.mId);
    pickingShader.SetUniform4m("uProjection", currState.mProjection);
    pickingShader.SetUniform3f("uViewPos", camera.mPosition);
    pickingShader.SetUniform4m("uView", view);
    pickingShader.SetUniform1i("uDisplayBoneIdx", 0);
    pickingShader.SetUniform4m("uBones", BoneTransforms);
    currModel->render(pickingShader, 0);

    glFlush();
    glFinish();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    u8 pickData[4];

    GLint pickingX = sceneCursorPos.X;
    GLint pickingY = framebuffer->mSize.Y - sceneCursorPos.Y;
    glReadPixels(pickingX, pickingY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pickData);
    pickedId = pickData[0] + (pickData[1] << 8) + (pickData[2] << 16);
    std::cout << "Id: " << pickedId << ", pickingX: " << pickingX
              << ", pickingY: " << pickingY << " cursorX: " << sceneCursorPos.X
              << ", cursorY: " << sceneCursorPos.Y << std::endl;
    // END PICKING PHASE ===========
    FramebufferGL::Bind(framebuffer->mId, clearColor, clearMask,
                        framebuffer->mSize);
    glUseProgram(riggedPhong.mId);
    riggedPhong.SetUniform4m("uProjection", currState.mProjection);
    riggedPhong.SetUniform3f("uViewPos", camera.mPosition);
    riggedPhong.SetUniform4m("uView", view);
    riggedPhong.SetUniform1i("uDisplayBoneIdx", 0);

    // NOTE(Jovan): Render model
    riggedPhong.SetUniform4m("uBones", BoneTransforms);
    currModel->render(riggedPhong, pickedId);

    FramebufferGL::Bind(0, clearColor, clearMask, currState.mWindow.mSize);
    glUseProgram(0);

    // NOTE(Jovan): UI RENDERING ========================================
    // NOTE(Jovan): New frame UI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // NOTE(Jovan): Main window
    ImVec2 Size(currState.mWindow.mSize.X, currState.mWindow.mSize.Y);
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(Size);
    ImGui::Begin(mainWindowName.c_str(), NULL, mainWindowFlags);
    ImGuiID DockID = ImGui::GetID("DockSpace");
    ImGui::DockSpace(DockID);

    if (!isMainWindowInitialized) {
      ImGui::DockBuilderRemoveNode(DockID);
      ImGui::DockBuilderAddNode(DockID, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(DockID, Size);
      ImGuiID LeftID = ImGui::DockBuilderSplitNode(DockID, ImGuiDir_Left, 0.5,
                                                   NULL, &DockID);
      ImGuiID BottomID = ImGui::DockBuilderSplitNode(LeftID, ImGuiDir_Down,
                                                     0.5f, NULL, &LeftID);
      ImGui::DockBuilderDockWindow("Model", LeftID);
      ImGui::DockBuilderDockWindow("Camera", BottomID);
      ImGui::DockBuilderDockWindow("Scene", DockID);
      ImGui::DockBuilderFinish(DockID);
      isMainWindowInitialized = true;
    }

    ImGui::End();

    // NOTE(Jovan): Scene window
    ImGui::Begin(sceneWindowName.c_str(), NULL,
                 ImGuiWindowFlags_AlwaysAutoResize);
    {
      ImGui::BeginChild("SceneRender");
      currState.mWindow.mSceneWindowFocused = ImGui::IsWindowFocused();

      ImVec2 windowSize = ImGui::GetWindowSize();
      if (currState.mFramebuffer.mSize.X != windowSize.x ||
          currState.mFramebuffer.mSize.Y != windowSize.y) {
        currState.mFramebuffer.Resize(windowSize.x, windowSize.y);
        resizePickingFramebuffer(pickingFramebuffer, pickingDepthBuffer,
                                 pickingTexture, windowSize.x, windowSize.y);
      }

      ImGui::Image((ImTextureID)currState.mFramebuffer.mTexture, windowSize,
                   ImVec2(0, 1), ImVec2(1, 0));

      ImVec2 imSceneWindowPos = ImGui::GetWindowPos();
      sceneWindowPos.X = imSceneWindowPos.x;
      sceneWindowPos.Y = imSceneWindowPos.y;

      ImGui::EndChild();
    }
    ImGui::End();

    // NOTE(Jovan): Model window
    ImGui::Begin(modelWindowName.c_str(), NULL,
                 ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Loaded model: %s", currModel->mFilePath.c_str());
    ImGui::Checkbox("Show bones", &currState.mShowBones);
    ImGui::Spacing();
    ImGui::DragFloat3("Position", modelPosition.Values, 1e-3f);
    ImGui::DragFloat3("Rotation", modelRotation.Values, 1e-1f);
    ImGui::DragFloat3("Scale", modelScale.Values, 1e-3f);
    ImGui::Spacing();
    ImGui::Text("Vertices: %d", currModel->mVerticesCount);
    ImGui::Spacing();
    ImGui::SliderFloat("Animation speed", &currModel->mActiveAnimation->mSpeed,
                       0.0f, 4.0f, "%.2f");
    ImGui::End();

    // NOTE(Jovan): Camera window
    ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::DragFloat3("Position", camera.mPosition.Values, 1e-3f);
    ImGui::Spacing();
    ImGui::DragFloat("FOV", &camera.mFOV, 1.0f, 15.0f, 120.0f, "%.1f");
    ImGui::Spacing();
    ImGui::Text("Pitch: %.2f", camera.mPitch * DEG);
    ImGui::Text("Yaw: %.2f", camera.mYaw * DEG);
    ImGui::End();

    // NOTE(Jovan): Render UI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    currState.EndFrame();
    timeInfo.frameEndMs = currentTimeInMillis();
    currState.mDT = timeInfo.getDeltaMs();

    glfwSwapBuffers(glfwWindow);
  }

  // NOTE(Jovan): Dispose UI
  ImGui_ImplGlfw_Shutdown();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(glfwWindow);
  glfwTerminate();
  return 0;
}
