#include "DescriptorSet.h"

#define ERROR_VOLATILE(x) x; if (getAlertSeverity() == FATAL) { return; }

#include "Device.h"

#include <array>

namespace Render
{
	int DescriptorSet::amountOfObjects = 0;

    DescriptorSet::DescriptorSet()
    {
		clear();
		amountOfObjects += 1;
		if (amountOfObjects > MAX_OBJECTS) {
			Alert("User created more descriptors than currently allowed.", CRITICAL);
			delete this;
		}
    }

    DescriptorSet::~DescriptorSet()
    {
		clear();
		amountOfObjects -= 1;
    }

	void DescriptorSet::init(size_t deviceUUID)
	{
		device = Request<Device>(deviceUUID, "self");

		for (auto descriptorResource : descriptorResources) {
			if (auto ptr = descriptorResource.lock()) {
				ptr->init(deviceUUID);
			}
		}
	}

	void DescriptorSet::destroy()
	{
		for (auto descriptorResource : descriptorResources) {
			if (auto ptr = descriptorResource.lock()) {
				ptr->destroy();
			}
		}
	}

	void DescriptorSet::create(VkDescriptorSetAllocateInfo info)
	{
		if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used", FATAL);
			return;
		}
		if (vkAllocateDescriptorSets((*device).getDevice(), &info, descriptorSets.data()) != VK_SUCCESS) {
			Alert("Failed to allocate descriptor sets!", FATAL);
			return;
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto descriptorWrites = getInfo(i);
			vkUpdateDescriptorSets((*device).getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void DescriptorSet::clear()
	{
		descriptorSets = {VK_NULL_HANDLE, VK_NULL_HANDLE};
	}

	void DescriptorSet::addDescriptorResource(std::weak_ptr<DescriptorResource>& descriptorResource)
	{
		descriptorResources.emplace_back(descriptorResource);
	}

	std::vector<VkWriteDescriptorSet> DescriptorSet::getInfo(int frame)
	{ // TODO: Logging with custom types, Dump info in object

		std::vector<VkWriteDescriptorSet> descriptorWrites;
		for (auto descriptorResource : descriptorResources) {
			if (auto ptr = descriptorResource.lock()) {
				descriptorWrites.emplace_back(ptr->createWrite(frame, descriptorSets[frame]));
			}
		}

		return descriptorWrites;
	}

	VkDescriptorSet& DescriptorSet::getDescriptorSet(uint32_t frame) 
	{		
		for (auto descriptorResource : descriptorResources) {
			if (auto ptr = descriptorResource.lock()) {
				ptr->update(frame);
			}
		}

		return descriptorSets[frame];
	}

	void DescriptorSet::record(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t frame)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &(getDescriptorSet(frame)), 0, nullptr);
	}
}