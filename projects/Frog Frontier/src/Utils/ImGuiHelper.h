#pragma once
// Include ImGui so it will be visible when we include this file
#include <imgui.h>

// Will be included in the CPP to avoid header bloat
struct GLFWwindow;

/// <summary>
/// Helper class for working with ImGui
/// </summary>
class ImGuiHelper {
public:
	/// <summary>
	/// Initializes the ImGui helper, should be called before doing any ImGui stuff
	/// </summary>
	static void Init(GLFWwindow* window);
	/// <summary>
	/// Cleans up the ImGui helper, should be called before closing the application
	/// </summary>
	static void Cleanup();

	/// <summary>
	/// Notifies ImGui that a new frame has begun
	/// Call at start of render loop
	/// </summary>
	static void StartFrame();

	/// <summary>
	/// Notifies ImGui that a frame has ended and should be rendered
	/// Call at end of render loop before glfwSwapBuffers
	/// </summary>
	static void EndFrame();

protected:
	ImGuiHelper() = default;

	static GLFWwindow* _window;
};

// Allows for an ImGui command to have a left aligned label instead of right aligned
// EX: LABEL_LEFT(ImGui::DragFloat3, "Label", &value);
#define LABEL_LEFT(func, label, ...) (ImGui::TextUnformatted(label), ImGui::SameLine(), func("##" label, __VA_ARGS__))