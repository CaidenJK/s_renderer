#include "TextureImage.h"

#include "Device.h"

namespace Render
{
    TextureImage::TextureImage()
    {
    }

    TextureImage::~TextureImage()
    {
        destroy();
    }

    void TextureImage::init(uint64_t deviceUUID)
    {
        ImageBuffer::init(deviceUUID);

        loadFromFile();
    }

    void TextureImage::destroy()
    {
        ImageBuffer::destroy();

        if (device) {
            if (imageSampler != VK_NULL_HANDLE) vkDestroySampler((*device).getDevice(), imageSampler, nullptr);
        }
    }

    VkDescriptorImageInfo TextureImage::getDescriptorInfo(int image)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = imageSampler;

        return imageInfo;
    }
    
    void TextureImage::loadFromFile()
    {
        file = Request<FILETYPE>(FILE_Request, filePath, {Manager::FileFlags::IMAGE | Manager::FileFlags::READ, 4});

        if (file.wait() != Manager::State::YES) {
            Alert("Failed to load image from file!", CRITICAL);
            return;
        }
        auto imageFile = dynamic_cast<Manager::ImageFile*>(*file);

        imageSize = static_cast<VkDeviceSize>(imageFile->width) *
                    static_cast<VkDeviceSize>(imageFile->height) *
                    4;

        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(imageFile->width, imageFile->height)))) + 1;

        loadImageToMemory(imageSize, imageFile);

        createImage(imageFile->width, imageFile->height, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(imageFile->width), static_cast<uint32_t>(imageFile->height));

        generateMipmaps(VK_FORMAT_R8G8B8A8_SRGB, imageFile->width, imageFile->height, mipLevels);

        vkDestroyBuffer((*device).getDevice(), stagingBuffer, nullptr);
        vkFreeMemory((*device).getDevice(), stagingBufferMemory, nullptr);
        stagingBuffer = VK_NULL_HANDLE, stagingBufferMemory = VK_NULL_HANDLE;

        createImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        createSampler();

        imageFile->close();
    }

    void TextureImage::loadImageToMemory(VkDeviceSize imageSize, Manager::ImageFile* file)
    {
        if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used.", FATAL);
            return;
		}

        (*device).createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory((*device).getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, file->pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory((*device).getDevice(), stagingBufferMemory);
    }

    void TextureImage::createSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties((*device).getPhysicalDevice(), &properties); // Expose this struct instead of physicalDevice

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f; // "Level of detail"
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler((*device).getDevice(), &samplerInfo, nullptr, &imageSampler) != VK_SUCCESS) {
            Alert("Failed to create texture sampler!", FATAL);
            return;
        }
    }
}