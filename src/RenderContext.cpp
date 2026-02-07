#include "RenderContext.h"

#include <chrono>

#define ERROR_CHECK if (getRenderErrorState()) { return; }
#define EXTERN_ERROR(x) if(x->getAlertSeverity() == StarryAsset::FATAL) { return; }

#define STARRY_RENDER_INITIALIZE_SUCCESS \
	"----------------------------------------\n" \
	"Starry Render initialized successfully!\n" \
	"----------------------------------------\n"

#define STARRY_RENDER_EXIT_SUCCESS \
	"----------------------------------------\n" \
	"Starry Render exited successfully!\n" \
	"----------------------------------------\n"

namespace Render {

	std::vector<DescriptorSetReservation> DescriptorInfo::decode(std::vector<DescriptorInfo> info)
	{
		std::vector<DescriptorSetReservation> sets;
		for (int i = 0; i < info.size(); i++) {
			DescriptorSetReservation reservation{};
			reservation.indices = i;
			reservation.count = info[i].count;
			if (info[i].type == UNIFORM_BUFFER) {
				reservation.stage = VK_SHADER_STAGE_VERTEX_BIT;
				reservation.types = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			}
			else if (info[i].type == IMAGE_SAMPLER) {
				reservation.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				reservation.types = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}

			sets.push_back(reservation);
		}

		return sets;
	}

	RenderConfig::RenderConfig(std::string vertShader, std::string fragShader, MSAAOptions msaa, 
			glm::vec3 clearColor, std::vector<DescriptorInfo> descriptorInfo, std::vector<PushConstantInfo> pushConstantInfo) :
		vertexShader(vertShader), fragmentShader(fragShader), msaaSamples((VkSampleCountFlagBits)msaa), 
			clearColor(clearColor),  descriptorInfo(descriptorInfo), pushConstantInfo(pushConstantInfo) {}

	RenderContext::~RenderContext() 
	{
		Destroy();
		if (!getErrorState()) {
			Alert(STARRY_RENDER_EXIT_SUCCESS, BANNER);
		}
	}

	void RenderContext::Init(std::shared_ptr<Window>& window, RenderConfig config)
	{
		m_state.isInitialized = false;

		m_config = config;

		auto setReservations = DescriptorInfo::decode(config.descriptorInfo);
		auto deviceConfig = DeviceConfig{ m_config.msaaSamples, m_config.clearColor, window, setReservations};
		m_renderDevice.init(deviceConfig);

		m_shaders.init(m_renderDevice.getUUID(), { config.vertexShader, config.fragmentShader });
		
		m_renderSwapchain.init(m_renderDevice.getUUID(), { window->getUUID() });
		m_renderPass.init(m_renderDevice.getUUID(), {m_renderSwapchain.getImageFormats()});

		PipelineConstructInfo info = { m_renderPass.getUUID(), m_shaders.getUUID(), m_pushConstant.getUUID()};
		m_renderPipeline.init(m_renderDevice.getUUID(), info);

		m_renderSwapchain.generateFramebuffers(m_renderPass.getRenderPass());

		m_window = window;

		m_masterBufferData = std::make_shared<Buffer>();
		m_masterBufferData->init(m_renderDevice.getUUID());
	}

	void RenderContext::Load(std::shared_ptr<DescriptorSet>& descriptorSet)
	{
		m_state.isInitialized = false;

		descriptorSet->init(m_renderDevice.getUUID());
		m_descriptorSets.push_back(descriptorSet);
	}

	void RenderContext::Load(std::shared_ptr<VertexBufferData>& buffer)
	{
		m_state.isInitialized = false;

		m_masterBufferData->loadData(*buffer);
	}

	void RenderContext::Load(std::shared_ptr<Canvas>& canvas)
	{
		m_state.isInitialized = false;

		if (auto window = m_window.lock()) {
			CanvasConstructInfo info{
				window->getUUID(),
				m_renderPass.getUUID(),
				m_renderSwapchain.getUUID()
			};

			canvas->init(m_renderDevice.getUUID(), info);
			m_cnvs = canvas;
		}
		else {
			Alert("Window ref is expired!", CRITICAL);
		}
	}

	void RenderContext::Ready()
	{
		for (auto& descriptorSet : m_descriptorSets) {
			if (auto ptr = descriptorSet.lock()) {
				m_renderDevice.createDescriptorSets(ptr);
			}
			else {
				Alert("One or more Descriptor Sets have expired.", WARNING);
			}
		}

		m_masterBufferData->finalize();

		m_state.isInitialized = true;
	}

	void RenderContext::recreateSwapchain()
	{
		if (auto wndw = m_window.lock()) {
			if (wndw->isWindowMinimized()) { return; }
		}
		else {
			Alert("Window reference expired.", FATAL);
			m_state.isInitialized = false;
			return;
		}
		WaitIdle();

		m_renderSwapchain.constructSwapChain();
		m_renderSwapchain.generateFramebuffers(m_renderPass.getRenderPass());
	}

	void RenderContext::checkSwapChainRecreation() 
	{
		bool framebufferResized = false;
		if (auto wndw = m_window.lock()) {
			framebufferResized = wndw->wasFramebufferResized();
		}
		else {
			Alert("Window reference expired.", FATAL);
			m_state.isInitialized = false;
			return;
		}

		if (framebufferResized || m_renderSwapchain.shouldRecreate()) {
			recreateSwapchain();
		}
	}

	void RenderContext::Draw()
	{
		if (!m_state.isInitialized) {
			Alert("Render Context not fully initialized before drawing!", FATAL);
			return;
		}
		checkSwapChainRecreation();

		DrawInfo drawInfo = {
			m_renderSwapchain,
			m_renderPass
		};
		
		m_renderDevice.beginFrame(drawInfo);
		m_renderDevice.startSwapChainRenderPass(drawInfo);

		// Start Record
		
		m_renderPipeline.record(drawInfo);

		m_pushConstant.record(drawInfo, m_renderPipeline.getPipelineLayout());

		auto numSubBuffers = m_masterBufferData->bind(drawInfo);

		for (int i = 0; i < numSubBuffers; i++) {
			if (auto descriptor = m_descriptorSets[i].lock()) {
				descriptor->record(drawInfo, m_renderPipeline.getPipelineLayout(), m_renderDevice.getCurrentFrame());
			}
			m_masterBufferData->recordSubBuffer(drawInfo, i);
		}

		if (auto canvas = m_cnvs.lock()) {
			canvas->record(drawInfo);
		}

		// End Record

		m_renderDevice.endSwapChainRenderPass(drawInfo);
		m_renderDevice.endFrame(drawInfo);
	}

	void RenderContext::Destroy()
	{
		m_state.isInitialized = false;
		m_renderDevice.waitIdle();
		
		m_shaders.destroy();
		m_renderSwapchain.destroy();
		m_renderPass.destroy();
		m_renderPipeline.destroy();

		m_masterBufferData->destroy();
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

		m_renderDevice.destroy();
	}
}