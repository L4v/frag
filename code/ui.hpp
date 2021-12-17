#ifndef UI_HPP
#define UI_HPP
// TODO(Jovan): Add different backend support via compiler flags
#include <string>
#include "include/imgui/imgui.h"
#include "include/imgui/imgui_internal.h"
#include "include/imgui/imgui_impl_glfw.h"
#include "include/imgui/imgui_impl_opengl3.h"
#include "frag.hpp"

void InitUI(GLFWwindow *window);
void NewFrameUI();
void RenderUI();
void DisposeUI();

class SceneWindow {
public:
    ImGuiWindowFlags mFlags;
    bool             mHasResized;
    std::string      mName;

    SceneWindow(const std::string &name, ImGuiWindowFlags flags);
    void Render(EngineState *state);
};

class MainWindow {
public:
    std::string      mName;
    ImGuiWindowFlags mFlags;
    bool             mInitialized;
    SceneWindow     *mSceneWindow;

    MainWindow(const std::string &name, ImGuiWindowFlags flags);
    void Render(EngineState *state, i32 width, i32 height);
};

#endif
