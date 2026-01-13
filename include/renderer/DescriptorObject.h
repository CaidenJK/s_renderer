#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <StarryManager.h>

#include <vector>

namespace Render
{
    class DescriptorObject
    {
        public:
            DescriptorObject() {}
            ~DescriptorObject() {}

            virtual void init(size_t deviceUUID) = 0;
            virtual void destroy() = 0;
            
            virtual VkWriteDescriptorSet createWrite(int frame, VkDescriptorSet& descriptorSet) = 0;
            virtual void update(int frame) {}
    };
}