#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <atomic>

#define DEFAULT_HEIGHT 600
#define DEFAULT_WIDTH 800
#define DEFAULT_TITLE "Starry"

#include <StarryManager.h>

namespace Render
{
	class GLFWDebugger : public Manager::StarryAsset {
		public:
			void registerDebugAlert(int error, const char* description);
			ASSET_NAME("GLFW Debugger");
	};
	
	class Window : public Manager::StarryAsset {
	public:
		Window(const char* title = DEFAULT_TITLE);
		~Window();

		Window operator=(const Window&) = delete;
		Window(const Window&) = delete;

		bool shouldClose() const;
		void pollEvents() const;
		GLFWwindow* getGLFWwindow() const;

		void createVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface);
		void getFramebufferSize(int& width, int& height);

		bool wasFramebufferResized() { return framebufferResized.load(); }
		void resetFramebufferResizedFlag() { framebufferResized.store(false); }

		bool isWindowMinimized();

		ASSET_NAME("Window")

		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	private:
		void initWindow();

		int width = DEFAULT_WIDTH;
		int height = DEFAULT_HEIGHT;
		const char* title;
		GLFWwindow* window;

		std::atomic<bool> framebufferResized{ false };

		static void glfwErrorCallback(int error, const char* description);
		static GLFWDebugger* debugger;
	};
}