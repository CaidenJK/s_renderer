#pragma once

#include <StarryManager.h>

#include <array>

#include "Window.h"
#include "Device.h"

#include "RenderLayout.h"

#define DEFAULT_SHADER_PATHS {}
#define STARRY_NO_DATA {}

namespace Render
{
	/*
		The Starry Render Engine API class. Manages and interfaces with the Vulkan rendering context.
	*/

	struct DescriptorInfo {
		enum DescriptorType {
			UNIFORM_BUFFER,
			IMAGE_SAMPLER
		};

		DescriptorType type;
		int count;

		static std::vector<DescriptorSetReservation> decode(std::vector<DescriptorInfo> info);
	};

	struct RenderConfig {
		enum MSAAOptions {
			MSAA_DISABLED = VK_SAMPLE_COUNT_1_BIT,
			MSAA_2X = VK_SAMPLE_COUNT_2_BIT,
			MSAA_4X = VK_SAMPLE_COUNT_4_BIT,
			MSAA_8X = VK_SAMPLE_COUNT_8_BIT,
			MSAA_16X = VK_SAMPLE_COUNT_16_BIT,
			MSAA_32X = VK_SAMPLE_COUNT_32_BIT,
			MSAA_64X = VK_SAMPLE_COUNT_64_BIT
		};

		VkSampleCountFlagBits msaaSamples;
		glm::vec3 clearColor;

		std::vector<DescriptorInfo> descriptorInfo;
		std::vector<PushConstantInfo> pushConstantInfo;

		RenderConfig(MSAAOptions msaa, 
			glm::vec3 clearColor, std::vector<DescriptorInfo> descriptorInfo, std::vector<PushConstantInfo> pushConstantInfo);
		RenderConfig() {}
	};

	struct RenderState {
		bool isInitialized = false;
	};

	class RenderContext : public Manager::StarryAsset
	{
	public:
		RenderContext() {}
		~RenderContext();

		void Init(std::shared_ptr<Window>& window, RenderConfig config);

		void Add(std::shared_ptr<RenderLayout> layout);

		void Ready();
		void Draw();

		void WaitIdle() { m_renderDevice.waitIdle(); }
		
		void Destroy();

		bool getErrorState() const { return Manager::AssetManager::get().lock()->isFatal(); }
		void dumpAlerts() const { Manager::AssetManager::get().lock()->isFatal(); }

		std::array<unsigned int, 2> getExtent() { return { m_renderSwapchain.getExtent().width, m_renderSwapchain.getExtent().height }; }

		ASSET_NAME("Render Context")
	private:
		void checkSwapChainRecreation();
		void recreateSwapchain();

		RenderState m_state;
		RenderConfig m_config;

		std::weak_ptr<Window> m_window;

		Device m_renderDevice{};
		SwapChain m_renderSwapchain{};
		RenderPass m_renderPass{};

		std::map<uint32_t, std::weak_ptr<RenderLayout>> m_layouts;
	};

	// call init
	// load resources
	// draw, finalize resources before first draw. If resources change, isinit = false
}