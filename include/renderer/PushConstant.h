#pragma once

#include <StarryManager.h>

#include "glm/glm.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Render
{
    struct PushConstantInfo {
        enum PushConstantType {
			MAT4,
			VEC4
		};
        enum ShaderStage {
			VERTEX,
			FRAGMENT
		};

		PushConstantType type;
		ShaderStage stage;

		static std::vector<VkPushConstantRange> decode(std::vector<PushConstantInfo> info);
	};

    struct PushConstantConstructInfo {
        std::vector<PushConstantInfo> layouts;
    };

    struct PushConstantData 
    {
        void* data;
        VkPushConstantRange layout;
    };

    class PushConstant : public Manager::StarryAsset 
    {
        public:
            PushConstant();
            ~PushConstant();

            void init(PushConstantConstructInfo info);
            void destroy();
            
            void record(VkCommandBuffer commandBuffer, VkPipelineLayout& layout);

            void addPushConstantData(void* data, int layoutIndex);

            std::vector<VkPushConstantRange> getPushConstantRanges();

            ASSET_NAME("Push Constant")
        private:
            std::vector<PushConstantData> pcData{};
    };
}