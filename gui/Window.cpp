#include "Window.h"
#include "../imgui/imgui.h"

void Window::draw()
{
    if (!pOpen)
        return;

    ImGuiWindowFlags flags = static_cast<ImGuiWindowFlags>(getWindowFlags());
    if (shouldBringToFront) {
        ImGui::SetNextWindowFocus();
        shouldBringToFront = false;
    }

    if (ImGui::Begin(name.c_str(), &pOpen, flags))
        onDraw();
    ImGui::End();
}

void Window::operator()()
{
    draw();
} 