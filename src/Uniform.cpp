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

	void Uniform::init(size_t deviceUUID)
	{
		device = Request<Device>(deviceUUID, "self");
		createUniforms();
	}

	void Uniform::destroy()
	{
		if (device) {
			for (size_t i = 0; i < uniforms.size(); i++) {
				vkDestroyBuffer((*device).getDevice(), uniforms[i], nullptr);
				uniforms[i] = VK_NULL_HANDLE;
				vkFreeMemory((*device).getDevice(), uniformsMemory[i], nullptr);
				uniformsMemory[i] = VK_NULL_HANDLE;
				uniformsMapped[i] = nullptr;
			}
		}
	}

	void Uniform::update(int frame)
	{
		memcpy(uniformsMapped[frame], &buffer, sizeof(buffer));
	}

	void Uniform::createUniforms()
	{
		VkDeviceSize bufferSize = sizeof(buffer);

		uniforms.resize(MAX_FRAMES_IN_FLIGHT);
		uniformsMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformsMapped.resize(MAX_FRAMES_IN_FLIGHT);

		if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used.", FATAL);
			return;
		}
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			(*device).createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniforms[i], uniformsMemory[i]);

			vkMapMemory((*device).getDevice(), uniformsMemory[i], 0, bufferSize, 0, &uniformsMapped[i]);
		}
	}

	VkWriteDescriptorSet Uniform::createWrite(int frame, VkDescriptorSet& descriptorSet)
	{
		bufferInfo.buffer = uniforms[frame];
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