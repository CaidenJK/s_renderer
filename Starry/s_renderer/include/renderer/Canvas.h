#pragma once

#include <StarryManager.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "Window.h"

namespace Render
{
    struct CanvasConstructInfo
    {
        size_t windowUUID;
        size_t pipelineUUID;
        size_t swapChainUUID;
        size_t descriptorUUID;
    };

    class Device;
    class Pipeline;
    class SwapChain;
    class Descriptor;

    class Canvas : public Manager::StarryAsset
    {
        public:
            Canvas();
            virtual ~Canvas();

            void init(size_t deviceUUID, CanvasConstructInfo info);
            void destroy();

            static bool isMouseControlledByImGUI();

            void PollEvents();
            void Record(VkCommandBuffer commandBuffer);

            virtual ASSET_NAME("Canvas");
        protected:
            virtual void Display() {};
        private:
            void initImGui();
            void setImGuiStyle();

            void constructImGuiInfo(Pipeline& pipeline, SwapChain& swapChain, Descriptor& descriptor);

            void StartDraw(); // Main thread
            void EndDraw(); // Main thread

            std::atomic<int> drawStage = 0;

            ImGui_ImplVulkan_InitInfo guiInfo{};
            ImDrawData* drawData = nullptr;

            Manager::ResourceHandle<Device> device;
            Manager::ResourceHandle<Window> window;
    };
}