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
		Alert("UNIFORM DESTRUCT!", CRITICAL);
		destroy();
	}

	void Uniform::init(size_t deviceUUID)
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

	void Uniform::update(int frame)
	{
		memcpy(UniformsMapped[frame], &buffer, sizeof(buffer));
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

	VkWriteDescriptorSet Uniform::createWrite(int frame, VkDescriptorSet& descriptorSet)
	{
		bufferInfo.buffer = Uniforms[frame];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformData);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional

		return descriptorWrite;
	}
}