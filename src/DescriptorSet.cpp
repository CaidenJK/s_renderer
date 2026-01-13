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

		if (descriptors.uniformObject.wait() != Manager::State::YES ||
			descriptors.textureObject.wait() != Manager::State::YES) {
			Alert("Failed to retrieve Descriptor Objects!", FATAL);
			return;
		}
		(*descriptors.uniformObject).init(deviceUUID);
		(*descriptors.textureObject).init(deviceUUID);
	}

	void DescriptorSet::destroy()
	{
		if (descriptors.uniformObject.wait() == Manager::State::YES) {
			(*descriptors.uniformObject).destroy();
		}
		if (descriptors.textureObject.wait() == Manager::State::YES) {
			(*descriptors.textureObject).destroy();
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
	}

	void DescriptorSet::clear()
	{
		descriptorSets = {VK_NULL_HANDLE, VK_NULL_HANDLE};
	}

	void DescriptorSet::addDescriptors(std::array<size_t, 2> objects)
	{
		descriptors.uniformObject = Request<Uniform>(objects[0], "self");
		descriptors.textureObject = Request<TextureImage>(objects[1], "self");
	}

	std::array<VkWriteDescriptorSet, 2> DescriptorSet::getInfo(int frame)
	{ // TODO: Logging with custom types, Dump info in object

		if (descriptors.uniformObject.wait() != Manager::State::YES ||
			descriptors.textureObject.wait() != Manager::State::YES) {
			Alert("Failed to retrieve Descriptor Objects!", FATAL);
			return {};
		}

		std::array<VkWriteDescriptorSet, 2> descriptorWrites;
		descriptorWrites[0] = (*descriptors.uniformObject).createWrite(frame, descriptorSets[frame]);
		descriptorWrites[1] = (*descriptors.textureObject).createWrite(frame, descriptorSets[frame]);

		return descriptorWrites;
	}

	VkDescriptorSet& DescriptorSet::getDescriptorSet(uint32_t frame) {
		if (descriptors.uniformObject.wait() != Manager::State::YES ||
			descriptors.textureObject.wait() != Manager::State::YES) {
			Alert("Failed to retrieve Descriptor Objects!", FATAL);
			return descriptorSets[frame];
		}
		
		(*descriptors.uniformObject).update(frame);
		(*descriptors.textureObject).update(frame);

		return descriptorSets[frame];
	}
}