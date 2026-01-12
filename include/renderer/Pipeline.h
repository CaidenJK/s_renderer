#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <memory>

#include <StarryManager.h>

#include "Buffer.h"
#include "Shader.h"
#include "Descriptor.h"
#include "SwapChain.h"

#define ERROR_VOLATILE(x) x; if (getAlertSeverity() == FATAL) { return; }

namespace Render 
{
	class Device;

	struct PipelineConstructInfo
	{
		uint64_t swapChainUUID;
		uint64_t descriptorUUID;
		uint64_t shaderUUID;
	};

	class Pipeline : public Manager::StarryAsset {
	public:
		Pipeline();
		~Pipeline();

		void init(uint64_t deviceUUID, PipelineConstructInfo info);
		void destroy();

		Pipeline operator=(const Pipeline&) = delete;
		Pipeline(const Pipeline&) = delete;

		VkRenderPass& getRenderPass() { return renderPass; }
		VkPipelineLayout& getPipelineLayout() { return pipelineLayout; }
		VkPipeline& getPipeline() { return graphicsPipeline; }

		ASSET_NAME("Pipeline")

	private:
		void constructPipeline(SwapChain& swapChain, Shader& shader, Descriptor& descriptor);
		void createRenderPass(SwapChain& swapChain);
		void constructPipelineLayout(Shader& shader, Descriptor& descriptor);

		VkPipelineVertexInputStateCreateInfo createVertexInputInfo();

		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;

		Manager::ResourceHandle<Device> device{};
	};
}