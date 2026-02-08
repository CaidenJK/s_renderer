#include "RenderLayout.h"

namespace Render
{
    RenderLayout::RenderLayout(LayoutConfig config) : config(config)
    {
        
    }

    RenderLayout::~RenderLayout()
    {

    }

    void RenderLayout::Init(LayoutInitInfo info)
    {
        Destroy();
        device = Request<Device>(info.deviceUUID, "self");
        if (device.wait() != Manager::State::YES) {
            Alert("Device died before it was ready to be used.", FATAL);
        }
        this->info = info;

        m_shaders.init(info.deviceUUID, { config.vertexShader, config.fragmentShader });

        PipelineConstructInfo constructInfo = { info.renderPassUUID, m_shaders.getUUID(), m_pushConstant.getUUID()};
		m_renderPipeline.init(info.deviceUUID, constructInfo);

		m_masterBufferData.init(info.deviceUUID);
    }

    void RenderLayout::Ready()
    {
		for (auto& descriptorSet : m_descriptorSets) {
			if (auto ptr = descriptorSet.lock()) {
				(*device).createDescriptorSets(ptr);
			}
			else {
				Alert("One or more Descriptor Sets have expired.", WARNING);
			}
		}

		m_masterBufferData.finalize();
    }

    void RenderLayout::Destroy()
    {
        m_shaders.destroy();
		m_renderPipeline.destroy();

		m_masterBufferData.destroy();
		m_pushConstant.destroy();

		for (auto& descriptorSet : m_descriptorSets) {
			if (auto ptr = descriptorSet.lock()) {
				ptr->destroy();
			}
			else {
				Alert("One or more Descriptor Sets have expired.", WARNING);
			}
		}

		if (auto canvas = m_cnvs.lock()) {
			canvas->destroy();
		}
    }

    void RenderLayout::Load(std::shared_ptr<DescriptorSet>& descriptorSet)
    {
        descriptorSet->init(info.deviceUUID);
		m_descriptorSets.push_back(descriptorSet);
    }

	void RenderLayout::Load(std::shared_ptr<VertexBufferData>& buffer)
    {
        m_masterBufferData.loadData(*buffer);
    }

	void RenderLayout::Load(std::shared_ptr<Canvas>& canvas)
    {
        CanvasConstructInfo constructInfo{
		    info.windowUUID,
			info.renderPassUUID,
			info.swapChainUUID
		};

		canvas->init(info.deviceUUID, constructInfo);
		m_cnvs = canvas;
    }

    void RenderLayout::Draw(DrawInfo& drawInfo)
    {
        // Start Record
		m_renderPipeline.record(drawInfo);

		m_pushConstant.record(drawInfo, m_renderPipeline.getPipelineLayout());

		auto numSubBuffers = m_masterBufferData.bind(drawInfo);

		for (int i = 0; i < numSubBuffers; i++) {
			if (auto descriptor = m_descriptorSets[i].lock()) {
				descriptor->record(drawInfo, m_renderPipeline.getPipelineLayout(), (*device).getCurrentFrame());
			}
			m_masterBufferData.recordSubBuffer(drawInfo, i);
		}

		if (auto canvas = m_cnvs.lock()) {
			canvas->record(drawInfo);
		}
		// End Record
    }
}