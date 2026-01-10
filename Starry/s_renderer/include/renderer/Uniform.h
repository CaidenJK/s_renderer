#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <StarryManager.h>

#include "glm/glm.hpp"

namespace Render 
{
	struct UniformData {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	class Device;

	class Uniform : public Manager::StarryAsset
	{
	public:
		Uniform();
		~Uniform();

		void init(uint64_t deviceUUID);
		void destroy();

		UniformData& getBuffer() { return buffer; }
		void setData(const UniformData& ubo) { buffer = ubo; }

		VkDescriptorBufferInfo getDescriptorInfo(int image);

		void updateUniform(uint32_t currentFrame);

		ASSET_NAME("Uniform Buffer")
	private:
		void createUniforms();

		UniformData buffer;

		std::vector<VkBuffer> Uniforms;
		std::vector<VkDeviceMemory> UniformsMemory;
		std::vector<void*> UniformsMapped;

		Manager::ResourceHandle<Device> device;
	};
}