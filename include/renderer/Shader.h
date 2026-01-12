#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <array>

#include <StarryManager.h>

namespace Render 
{
	class Device;

	struct ShaderConstructInfo
	{
		std::string& vertexShaderPath;
		std::string& fragmentShaderPath;
	};

	class Shader : public Manager::StarryAsset {
		public:
			Shader();
			~Shader();

			void init(uint64_t deviceUUID, ShaderConstructInfo info);
			void destroy();

			std::array<VkPipelineShaderStageCreateInfo, 2>& getShaderStages() { return shaderStages; }

			ASSET_NAME("Shader")

		private:
			VkShaderModule createShaderModule(const std::vector<char>& code, bool& error);
			std::vector<char> readFile(const std::string& filename, bool& error);

			void initShader();

			void bindShaderStages();

			void loadVertexShaderFromFile();
			void loadFragmentShaderFromFile();

			std::string vertexShaderPath;
			std::string fragmentShaderPath;

			std::vector<char> vertexShaderCode = {};
			std::vector<char> fragmentShaderCode = {};

			VkShaderModule vertShaderModule = VK_NULL_HANDLE;
			VkShaderModule fragShaderModule = VK_NULL_HANDLE;

			std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};

			Manager::ResourceHandle<Device> device{};
	};
}