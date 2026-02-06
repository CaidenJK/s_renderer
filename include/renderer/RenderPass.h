#pragma once

#include <StarryManager.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Render
{
    class Device;

    struct RenderPassConstructInfo
    {
        std::array<VkFormat, 2> swapChainImageFormats;
    };

    class RenderPass : public Manager::StarryAsset
    {
        public:
            RenderPass();
            ~RenderPass();

            void init(size_t deviceUUID, RenderPassConstructInfo info);
            void destroy();

            VkRenderPass& getRenderPass() { return renderPass; }

            ASSET_NAME("Render Pass")
        
        private:
            void constructRenderPass(std::array<VkFormat, 2>& swapChainImageFormats);
            VkRenderPass renderPass = VK_NULL_HANDLE;

            Manager::ResourceHandle<Device> device{};
    };
}