#include "ui.hpp"

void
InitUI(GLFWwindow *window) {
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void
NewFrameUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void
RenderUI() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void
DisposeUI() {
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

ModelWindow::ModelWindow(const std::string &name, ImGuiWindowFlags flags) {
    mName = name;
    mFlags = flags;
}

void
ModelWindow::Render(const std::string &filename, r32 *position, r32 *rotation, r32 *scale) {
    ImGui::Begin(mName.c_str(), NULL, mFlags);
    ImGui::Text("Loaded model: %s", filename.c_str());
    ImGui::Spacing();
    ImGui::DragFloat3("Position", position, 1e-3f);
    ImGui::DragFloat3("Rotation", rotation, 1e-1f);
    ImGui::DragFloat3("Scale", scale, 1e-3f);
    ImGui::End();
}

MainWindow::MainWindow(const std::string &name, ImGuiWindowFlags flags) {
    mName = name;
    mFlags = flags;
    mInitialized = false;
}

void
MainWindow::Render(EngineState *state, i32 width, i32 height) {
    ImVec2 Size(width, height);
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(Size);
    ImGui::Begin(mName.c_str(), NULL, mFlags);
    ImVec2 Pos(0.0f, 0.0f);
    ImGuiID DockID = ImGui::GetID("DockSpace");
    ImGui::DockSpace(DockID);

    if (!mInitialized) {
        ImGui::DockBuilderRemoveNode(DockID);
        ImGui::DockBuilderAddNode(DockID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(DockID, Size);

        ImGuiID LeftID = ImGui::DockBuilderSplitNode(DockID, ImGuiDir_Left, 0.4, NULL, &DockID);
        ImGuiID BottomID = ImGui::DockBuilderSplitNode(DockID, ImGuiDir_Down, 0.2f, NULL, &DockID);
        ImGui::DockBuilderDockWindow("Model", LeftID);
        ImGui::DockBuilderDockWindow("Camera", BottomID);
        ImGui::DockBuilderDockWindow("Scene",  DockID);
        ImGui::DockBuilderFinish(DockID);
        mInitialized = true;
    }

    ImGui::End();
}

SceneWindow::SceneWindow(const std::string &name, ImGuiWindowFlags flags) {
    mName = name;
    mFlags = flags;
}

void
SceneWindow::Render(EngineState *state) {
    ImGui::Begin(mName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize); {
        ImGui::BeginChild("SceneRender");
        state->mSceneWindowFocused = ImGui::IsWindowFocused();
        ImVec2 CursorPos = ImGui::GetIO().MousePos;
        if(state->mFirstMouse) {
            state->mCursorPos.x = CursorPos.x;
            state->mCursorPos.y = CursorPos.y;
            state->mFirstMouse = false;
        }
        r32 DX = state->mCursorPos.x - CursorPos.x;
        r32 DY = CursorPos.y - state->mCursorPos.y;
        state->mCursorPos.x = CursorPos.x;
        state->mCursorPos.y = CursorPos.y;

        if(state->mLeftMouse && state->mSceneWindowFocused) {
            state->mCamera->Rotate(DX, DY, state->mDT);
        }

        ImVec2 WindowSize = ImGui::GetWindowSize();
        if (state->mFramebufferSize.x != WindowSize.x || state->mFramebufferSize.y != WindowSize.y) {
            mHasResized = true;
        }
        state->mFramebufferSize.x = WindowSize.x;
        state->mFramebufferSize.y = WindowSize.y;

        ImGui::Image((ImTextureID)state->mFBOTexture, WindowSize, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::EndChild();
    } ImGui::End();
}
