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

	RenderConfig::RenderConfig(MSAAOptions msaa, 
			glm::vec3 clearColor, std::vector<DescriptorInfo> descriptorInfo, std::vector<PushConstantInfo> pushConstantInfo) :
			msaaSamples((VkSampleCountFlagBits)msaa), clearColor(clearColor),  descriptorInfo(descriptorInfo), 
			pushConstantInfo(pushConstantInfo) {}

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
		
		m_renderSwapchain.init(m_renderDevice.getUUID(), { window->getUUID() });
		m_renderPass.init(m_renderDevice.getUUID(), {m_renderSwapchain.getImageFormats()});

		m_renderSwapchain.generateFramebuffers(m_renderPass.getRenderPass());

		m_window = window;

		for (auto it = m_layouts.begin(); it != m_layouts.end(); ++it) {
			if (auto lyt = it->second.lock()) {
				lyt->Init({m_renderDevice.getUUID(), m_window.lock()->getUUID(), m_renderSwapchain.getUUID(), m_renderPass.getUUID()});
			}
			else {
				m_layouts.erase(it);
			}
		}
	}

	void RenderContext::Add(std::shared_ptr<RenderLayout> layout)
	{
		m_state.isInitialized = false;
		m_layouts.insert({layout->getPriority(), layout});
	}

	void RenderContext::Ready()
	{
		for (auto it = m_layouts.begin(); it != m_layouts.end(); ++it) {
			if (auto lyt = it->second.lock()) {
				lyt->Ready();
			}
			else {
				m_layouts.erase(it);
			}
		}

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
		if (m_renderSwapchain.shouldRecreate()) return;

		m_renderDevice.startSwapChainRenderPass(drawInfo);

		for (auto it = m_layouts.begin(); it != m_layouts.end(); ++it) {
			if (auto lyt = it->second.lock()) {
				lyt->Draw(drawInfo);
			}
			else {
				m_layouts.erase(it);
			}
		}

		m_renderDevice.endSwapChainRenderPass(drawInfo);
		m_renderDevice.endFrame(drawInfo);
	}

	void RenderContext::Destroy()
	{
		m_state.isInitialized = false;
		m_renderDevice.waitIdle();
		
		m_renderSwapchain.destroy();
		m_renderPass.destroy();

		for (auto layout : m_layouts) {
			if (auto lyt = layout.second.lock()) {
				lyt->Destroy();
			}
		}
		m_layouts.clear();

		m_renderDevice.destroy();
	}
}