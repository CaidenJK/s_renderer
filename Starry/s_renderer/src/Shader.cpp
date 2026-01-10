#include "Shader.h"

#include "Device.h"

#define ERROR_VOLATILE(x) x; if (getAlertSeverity() == FATAL) { return; }

namespace Render 
{
	Shader::Shader()
	{
	}

	Shader::~Shader()
	{
		destroy();
	}

	void Shader::init(uint64_t deviceUUID, ShaderConstructInfo info)
	{
		vertexShaderPath = info.vertexShaderPath;
		fragmentShaderPath = info.fragmentShaderPath;

		device = Request<Device>(deviceUUID, "self");
		initShader();
	}

	void Shader::destroy()
	{
		if (device) {
			if (vertShaderModule != VK_NULL_HANDLE) {
				vkDestroyShaderModule((*device).getDevice(), vertShaderModule, nullptr);
				vertShaderModule = VK_NULL_HANDLE;
			}
			if (fragShaderModule != VK_NULL_HANDLE) {
				vkDestroyShaderModule((*device).getDevice(), fragShaderModule, nullptr);
				fragShaderModule = VK_NULL_HANDLE;
			}
		}
	}

	void Shader::initShader() 
	{
		ERROR_VOLATILE(loadVertexShaderFromFile());
		ERROR_VOLATILE(loadFragmentShaderFromFile());

		bool error = false;
		
		if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used.", CRITICAL);
			return;
		}

		vertShaderModule = createShaderModule(vertexShaderCode, error);
		if (error) {
			Alert("Failed to create vertex shader module!", FATAL);
			return;
		}

		fragShaderModule = createShaderModule(fragmentShaderCode, error);
		if (error) {
			Alert("Failed to create vertex shader module!", FATAL);
			return;
		}

		ERROR_VOLATILE(bindShaderStages());
	}

	VkShaderModule Shader::createShaderModule(const std::vector<char>& code, bool& error) 
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule((*device).getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			error = true;
			return {};
		}
		error = false;
		return shaderModule;
	}

	void Shader::bindShaderStages() 
	{
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
	}

	void Shader::loadVertexShaderFromFile() 
	{
		bool error = false;
		vertexShaderCode = readFile(vertexShaderPath, error);
		if (error) {
			Alert("Failed to read vertex shader file: " + vertexShaderPath, FATAL);
		}
	}

	void Shader::loadFragmentShaderFromFile() 
	{
		bool error = false;
		fragmentShaderCode = readFile(fragmentShaderPath, error);
		if (error) {
			Alert("Failed to read fragment shader file: " + fragmentShaderPath, FATAL);
		}
	}

	std::vector<char> Shader::readFile(const std::string& filename, bool& error) 
	{
		auto file = Request<FILETYPE>(FILE_Request, filename, { Manager::FileFlags::READ | Manager::FileFlags::APPEND_START | Manager::FileFlags::BINARY });

		if (file.wait() != Manager::State::YES) {
			error = true;
			return {};
		}

		size_t fileSize = (size_t)(*file)->file.tellg();
		std::vector<char> buffer(fileSize);
		(*file)->file.seekg(0);
		(*file)->file.read(buffer.data(), fileSize);

		(*file)->close();

		error = false;
		return buffer;
	}
}