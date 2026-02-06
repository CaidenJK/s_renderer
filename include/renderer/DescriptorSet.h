#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <StarryManager.h>

#include <vector>

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_OBJECTS 64

#include "DescriptorResource.h"

namespace Render
{
    class Device;

    class DescriptorSet : public Manager::StarryAsset
    {
        public:
            DescriptorSet();
            ~DescriptorSet();

            virtual void init(size_t deviceUUID);

            void destroy();

            void create(VkDescriptorSetAllocateInfo allocInfo);
            void clear();

            std::vector<VkWriteDescriptorSet> getInfo(int frame);
            
            void addDescriptorResource(std::weak_ptr<DescriptorResource>& descriptorResource);
		    VkDescriptorSet& getDescriptorSet(uint32_t frame);

            void bind(VkCommandBuffer& commandBuffer, int frame);

            virtual ASSET_NAME("Descriptor Set")
        protected:
		    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

            Manager::ResourceHandle<Device> device;

            std::vector<std::weak_ptr<DescriptorResource>> descriptorResources;

            static int amountOfObjects;
    };
}