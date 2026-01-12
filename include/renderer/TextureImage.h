#pragma once 

#include <StarryManager.h>
#include "ImageBuffer.h"

namespace Render
{
    class Device;

    class TextureImage : public ImageBuffer
    {
        public:
            TextureImage();
            ~TextureImage();

            void init(uint64_t deviceUUID) override;
            void destroy() override;

			void storeFilePath(const std::string& path) { filePath = path; }
            void loadFromFile();

            VkDescriptorImageInfo getDescriptorInfo(int image);

            ASSET_NAME("TextureImage")
        private:
            void loadImageToMemory(VkDeviceSize imageSize, Manager::ImageFile* file);

            void createSampler();

            uint32_t mipLevels = 0;

            std::string filePath;
            Manager::ResourceHandle<FILETYPE> file;

            VkBuffer stagingBuffer = VK_NULL_HANDLE;
            VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
            
            VkSampler imageSampler = VK_NULL_HANDLE;
    };
}