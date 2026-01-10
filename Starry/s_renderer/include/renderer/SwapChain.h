#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <memory>
#include <optional>
#include <array>

#include <StarryManager.h>

#include "Window.h"
#include "ImageBuffer.h"

namespace Render 
{
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	class Device;

	struct SwapChainConstructInfo
	{
		uint64_t windowUUID;
	};

	class SwapChain : public Manager::StarryAsset {
		public:
			struct SwapChainSupportDetails {
				VkSurfaceCapabilitiesKHR capabilities;
				std::vector<VkSurfaceFormatKHR> formats;
				std::vector<VkPresentModeKHR> presentModes;

				VkFormat depthBufferFormat;
			};
		public:
			SwapChain();
			~SwapChain();

			SwapChain operator=(const SwapChain&) = delete;
			SwapChain(const SwapChain&) = delete;

			void init(uint64_t deviceUUID, SwapChainConstructInfo info);
			void destroy();

			void constructSwapChain();
			void generateFramebuffers(VkRenderPass& renderPass);

			void needRecreate();
			bool shouldRecreate() { return recreate; }

			std::vector<VkFramebuffer>& getFramebuffers() { return swapChainFramebuffers; }
			VkSwapchainKHR& getSwapChain() { return swapChain; }
			std::array<VkFormat, 2>& getImageFormats() { return imageFormats; }
			VkExtent2D& getExtent() { return swapChainExtent; }
			size_t getImageCount() { return swapChainImageBuffers.size(); }

			ASSET_NAME("Swapchain")

			static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice& device, VkSurfaceKHR surface);
		private:
			bool recreate = true;

			void createSwapChain(SwapChainSupportDetails& swapChainSupport, QueueFamilyIndices& indices, VkSurfaceKHR& surface);
			void createImageViews();

			static VkFormat findSupportedFormat(VkPhysicalDevice& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
            static VkFormat findDepthFormat(VkPhysicalDevice& device);
			bool hasStencilComponent(VkFormat format);

			void createColorResources();
			void createDepthResources();
			
			void cleanupSwapChain();

			VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
			VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
			VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

			VkSwapchainKHR swapChain = VK_NULL_HANDLE;

			std::vector<ImageBuffer> swapChainImageBuffers;
			std::shared_ptr<ImageBuffer> colorBuffer = nullptr;

			VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

			std::array<VkFormat, 2> imageFormats;
			VkExtent2D swapChainExtent = {0, 0};

			std::shared_ptr<ImageBuffer> depthBuffer;

			std::vector<VkFramebuffer> swapChainFramebuffers;

			Manager::ResourceHandle<Device> device{};
			Manager::ResourceHandle<Window> window{};
	};
}