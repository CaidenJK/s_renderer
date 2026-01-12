#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <StarryManager.h>

#include <vector>

#include "Uniform.h"
#include "TextureImage.h"

#define MAX_FRAMES_IN_FLIGHT 2

namespace Render
{
    class Device;

    class Descriptor : public Manager::StarryAsset
    {
        public:
            Descriptor();
            ~Descriptor();

            void init(uint64_t deviceUUID);
            void destroy();
            
            VkDescriptorSetLayout& getDescriptorSetLayout() { return descriptorSetLayout; }
		    VkDescriptorSet& getDescriptorSet(uint32_t index) { return descriptorSets[index]; }

            void createDescriptorSets(uint64_t ubUUID, uint64_t txUUID);

            VkDescriptorPool getPool() { return descriptorPool; }

            ASSET_NAME("Descriptor")
        private:
            void createDescriptorSetLayout();
            void createDescriptorPool();

            VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

		    std::vector<VkDescriptorSet> descriptorSets;

            Manager::ResourceHandle<Device> device;
    };
}