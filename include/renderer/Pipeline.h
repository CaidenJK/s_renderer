#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <memory>

#include <StarryManager.h>

#include "Buffer.h"
#include "Shader.h"
#include "RenderPass.h"
#include "PushConstant.h"

#define ERROR_VOLATILE(x) x; if (getAlertSeverity() == FATAL) { return; }

namespace Render 
{
	class Device;

	struct PipelineConstructInfo
	{
		size_t renderPassUUID;
		size_t shaderUUID;
		size_t pushConstantUUID;
	};

	class Pipeline : public Manager::StarryAsset {
	public:
		Pipeline();
		~Pipeline();

		void init(size_t deviceUUID, PipelineConstructInfo info);
		void destroy();

		Pipeline operator=(const Pipeline&) = delete;
		Pipeline(const Pipeline&) = delete;

		VkPipelineLayout& getPipelineLayout() { return pipelineLayout; }
		VkPipeline& getPipeline() { return graphicsPipeline; }

		void record(VkCommandBuffer commandBuffer);

		ASSET_NAME("Pipeline")

	private:
		void constructPipelineLayout(RenderPass& renderPass, Shader& shader, PushConstant& pushConstant);

		VkPipelineVertexInputStateCreateInfo createVertexInputInfo();

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;

		Manager::ResourceHandle<Device> device{};
	};
}