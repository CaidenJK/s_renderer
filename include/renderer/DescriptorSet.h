#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <StarryManager.h>

#include <vector>

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_OBJECTS 100

#include "DescriptorObject.h"
#include "Uniform.h"
#include "TextureImage.h"

namespace Render
{
    class Device;

    struct Descriptors
    {
        Manager::ResourceHandle<Uniform> uniformObject;
        Manager::ResourceHandle<TextureImage> textureObject;
    };

    class DescriptorSet : public Manager::StarryAsset
    {
        public:
            DescriptorSet();
            ~DescriptorSet();

            virtual void init(size_t deviceUUID);

            void destroy();

            void create(VkDescriptorSetAllocateInfo allocInfo);
            void clear();

            std::array<VkWriteDescriptorSet, 2> getInfo(int frame);
            
            void addDescriptors(std::array<size_t, 2> objects);
		    VkDescriptorSet& getDescriptorSet(uint32_t frame);

            void bind(VkCommandBuffer& commandBuffer, int frame);

            virtual ASSET_NAME("Descriptor Set")
        protected:
		    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

            Manager::ResourceHandle<Device> device;

            Descriptors descriptors;

            static int amountOfObjects;
    };
}