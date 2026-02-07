#include "PushConstant.h"

namespace Render
{
    std::vector<VkPushConstantRange> PushConstantInfo::decode(std::vector<PushConstantInfo> info)
	{
		std::vector<VkPushConstantRange> ranges;
		int totalSize = 0;
		for (auto pcInfo : info) {
			VkPushConstantRange range{};
			range.offset = totalSize;
			if (pcInfo.stage == VERTEX) {
				range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			}
			else if (pcInfo.stage == FRAGMENT) {
				range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			if (pcInfo.type == MAT4) {
				range.size = sizeof(glm::mat4);
			}
			else if (pcInfo.type == VEC4) {
				range.size = sizeof(glm::vec4);
			}

			totalSize += range.size;
			ranges.push_back(range);
		}

		return ranges;
	}

    PushConstant::PushConstant() {}
    PushConstant::~PushConstant() {}

    void PushConstant::init(PushConstantConstructInfo info) {
        auto layouts = PushConstantInfo::decode(info.layouts);

        for (auto layout : layouts) {
            pcData.emplace_back(nullptr, layout);
        }
    }

    void PushConstant::destroy()
    {
        pcData.clear();
    }

    void PushConstant::record(VkCommandBuffer commandBuffer, VkPipelineLayout& layout)
    {
        for (auto data : pcData) {
            if (data.data) vkCmdPushConstants(commandBuffer, layout, 
                data.layout.stageFlags, data.layout.offset, data.layout.size, data.data);
        }
    }

    std::vector<VkPushConstantRange> PushConstant::getPushConstantRanges() {
        std::vector<VkPushConstantRange> ranges{};
        for (auto data : pcData) {
            ranges.push_back(data.layout);
        }

        return ranges;
    }

    void PushConstant::addPushConstantData(void* data, int layoutIndex)
    {
        pcData[layoutIndex].data = data;
    }
}