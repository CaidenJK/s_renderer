#pragma once

#include <StarryManager.h>

#include "Uniform.h"
#include "Buffer.h"
#include "ImageBuffer.h"
#include "TextureImage.h"
#include "PushConstant.h"

#include "Canvas.h"

#include "Device.h"

namespace Render
{    

    enum DrawPriority {
        VERY_HIGH = 0,
        HIGH = 1,
        REGULAR = 2,
        LOW = 3,
        NONE = 4
    };

    struct LayoutConfig 
    {
        std::string vertexShader;
		std::string fragmentShader;

        DrawPriority priority = REGULAR;
    };

    struct LayoutInitInfo
    {
        size_t deviceUUID;
        size_t windowUUID;
        size_t swapChainUUID;
        size_t renderPassUUID;
    };

    class RenderLayout : Manager::StarryAsset
    {
        public:
            RenderLayout(LayoutConfig config);
            ~RenderLayout();

            void Init(LayoutInitInfo info);
            void Ready();
            void Destroy();

            void Load(std::shared_ptr<DescriptorSet>& descriptorSet);
		    void Load(std::shared_ptr<VertexBufferData>& buffer);
		    void Load(std::shared_ptr<Canvas>& canvas);

            void Draw(DrawInfo& drawInfo);

		    void UpdatePushConstants(void* data, int layoutIndex) { m_pushConstant.addPushConstantData(data, layoutIndex);}

            DrawPriority getPriority() { return config.priority; }

            ASSET_NAME("Render Layout")

        private:
            Pipeline m_renderPipeline{};
		    Shader m_shaders{};
		    PushConstant m_pushConstant{};

		    Buffer m_masterBufferData{};

            LayoutConfig config;
            LayoutInitInfo info;

		    std::vector<std::weak_ptr<DescriptorSet>> m_descriptorSets;
		    std::weak_ptr<Canvas> m_cnvs;

            Manager::ResourceHandle<Device> device{};
    };
}