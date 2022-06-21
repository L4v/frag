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
ModelWindow::Render(State *state, const std::string &filename, r32 *position, r32 *rotation, r32 *scale, i32 vertexCount) {
    ImGui::Begin(mName.c_str(), NULL, mFlags);
    ImGui::Text("Loaded model: %s", filename.c_str());
    ImGui::Spacing();
    ImGui::DragFloat3("Position", position, 1e-3f);
    ImGui::DragFloat3("Rotation", rotation, 1e-1f);
    ImGui::DragFloat3("Scale", scale, 1e-3f);
    ImGui::Spacing();
    ImGui::Text("Vertices: %d", vertexCount);
    ImGui::Spacing();
    ImGui::Text("Cursor pos: %f, %f", state->GetOldInput().mMouse.mCursorPos.X, state->GetOldInput().mMouse.mCursorPos.Y);
    ImGui::End();
}

MainWindow::MainWindow(const std::string &name, ImGuiWindowFlags flags) {
    mName = name;
    mFlags = flags;
    mInitialized = false;
}

void
MainWindow::Render(State *state, i32 width, i32 height) {
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
SceneWindow::Render(State *state) {
    ImGui::Begin(mName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize); {
        ImGui::BeginChild("SceneRender");
        state->mWindow.mSceneWindowFocused = ImGui::IsWindowFocused();

        ImVec2 WindowSize = ImGui::GetWindowSize();
        if (state->mFramebufferSize.X != WindowSize.x || state->mFramebufferSize.Y != WindowSize.y) {
            mHasResized = true;
        }
        state->mFramebufferSize.X = WindowSize.x;
        state->mFramebufferSize.Y = WindowSize.y;

        ImGui::Image((ImTextureID)state->mWindow.mFramebuffer.mTexture, WindowSize, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::EndChild();
    } ImGui::End();
}
