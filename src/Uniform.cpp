#include "Uniform.h"

#define ERROR_VOLATILE(x) x; if (getAlertSeverity() == FATAL) { return; }

#include "Device.h"

namespace Render
{
	Uniform::Uniform()
	{
	}
	
	Uniform::~Uniform()
	{
		destroy();
	}

	void Uniform::init(uint64_t deviceUUID)
	{
		device = Request<Device>(deviceUUID, "self");

		createUniforms();
	}

	void Uniform::destroy()
	{
		if (device) {
			for (size_t i = 0; i < Uniforms.size(); i++) {
				vkDestroyBuffer((*device).getDevice(), Uniforms[i], nullptr);
				vkFreeMemory((*device).getDevice(), UniformsMemory[i], nullptr);
			}
		}
	}

	VkDescriptorBufferInfo Uniform::getDescriptorInfo(int image)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = Uniforms[image];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformData);

		return bufferInfo;
	}

	void Uniform::updateUniform(uint32_t currentFrame) 
	{
		memcpy(UniformsMapped[currentFrame], &buffer, sizeof(buffer));
	}

	void Uniform::createUniforms()
	{
		VkDeviceSize bufferSize = sizeof(buffer);

		Uniforms.resize(MAX_FRAMES_IN_FLIGHT);
		UniformsMemory.resize(MAX_FRAMES_IN_FLIGHT);
		UniformsMapped.resize(MAX_FRAMES_IN_FLIGHT);

		if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used.", FATAL);
			return;
		}
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			(*device).createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Uniforms[i], UniformsMemory[i]);

			vkMapMemory((*device).getDevice(), UniformsMemory[i], 0, bufferSize, 0, &UniformsMapped[i]);
		}
	}
}