#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <StarryManager.h>

#include "glm/glm.hpp"

#include "DescriptorResource.h"

namespace Render 
{
	struct UniformData {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	class Device;

	class Uniform : public Manager::StarryAsset, public DescriptorResource
	{
	public:
		Uniform();
		~Uniform();

		void init(size_t deviceUUID) override;
		void destroy() override;

		UniformData& getBuffer() { return buffer; }
		void setData(const UniformData& ubo) { buffer = ubo; }

		void update(int frame) override;

		VkWriteDescriptorSet createWrite(int frame, VkDescriptorSet& descriptorSet) override;

		const std::string getAssetName() override {	return "Uniform Buffer"; }
	private:
		void createUniforms();

		UniformData buffer;

		std::vector<VkBuffer> Uniforms;
		std::vector<VkDeviceMemory> UniformsMemory;
		std::vector<void*> UniformsMapped;

		VkDescriptorBufferInfo bufferInfo{};

		Manager::ResourceHandle<Device> device;
	};
}