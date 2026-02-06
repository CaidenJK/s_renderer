#pragma once

#include <StarryManager.h>

#include <array>

#include "Window.h"
#include "Device.h"
#include "Uniform.h"
#include "Buffer.h"
#include "ImageBuffer.h"
#include "TextureImage.h"

#include "Canvas.h"

#define DEFAULT_SHADER_PATHS {}

namespace Render
{
	/*
		The Starry Render Engine API class. Manages and interfaces with the Vulkan rendering context.
	*/

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

		std::shared_ptr<Window> window = nullptr;

		std::string vertexShader;
		std::string fragmentShader;

		VkSampleCountFlagBits msaaSamples;
		glm::vec3 clearColor;

		RenderConfig(std::string vertShader, std::string fragShader, MSAAOptions msaa, glm::vec3 clearColor);
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

		void Load(std::shared_ptr<DescriptorSet>& descriptorSet);
		void Load(std::shared_ptr<VertexBufferData>& buffer);

		void Load(std::shared_ptr<Canvas>& canvas);
		
		void Ready();
		void Draw();

		void WaitIdle() { m_renderDevice.waitIdle(); }
		
		void Destroy();

		bool getErrorState() const { return Manager::AssetManager::get().lock()->isFatal(); }
		void dumpAlerts() const { Manager::AssetManager::get().lock()->isFatal(); }

		std::array<unsigned int, 2> getExtent() { return { m_renderSwapchain.getExtent().width, m_renderSwapchain.getExtent().height }; }

		ASSET_NAME("Render Context")
	private:
		void recreateSwapchain();

		RenderState m_state;
		RenderConfig m_config;

		std::weak_ptr<Window> m_window;

		Device m_renderDevice{};
		SwapChain m_renderSwapchain{};
		RenderPass m_renderPass{};
		Pipeline m_renderPipeline{};
		Shader m_shaders{};

		std::shared_ptr<Buffer> m_masterBufferData;

		// extern
		std::vector<std::weak_ptr<DescriptorSet>> m_descriptorSets;
		std::weak_ptr<Buffer> m_buffer;
		std::weak_ptr<Canvas> m_cnvs;
	};

	// call init
	// load resources
	// draw, finalize resources before first draw. If resources change, isinit = false

}