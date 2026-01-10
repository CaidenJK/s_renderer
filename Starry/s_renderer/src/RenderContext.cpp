#include "RenderContext.h"

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

	RenderConfig::RenderConfig(std::string vertShader, std::string fragShader, MSAAOptions msaa, glm::vec3 clearColor) :
		vertexShader(vertShader), fragmentShader(fragShader), msaaSamples((VkSampleCountFlagBits)msaa), clearColor(clearColor) {}

	RenderContext::~RenderContext() 
	{
		Destroy();
		if (!getErrorState()) {
			Alert(STARRY_RENDER_EXIT_SUCCESS, BANNER);
		}
	}

	void RenderContext::Init(std::shared_ptr<Window>& window, RenderConfig config)
	{
		m_config = config;
		auto deviceConfig = DeviceConfig{ m_config.msaaSamples, m_config.clearColor, window};
		m_renderDevice.init(deviceConfig);

		m_shaders.init(m_renderDevice.getUUID(), { config.vertexShader, config.fragmentShader });
		m_descriptor.init(m_renderDevice.getUUID());
		
		m_renderSwapchain.init(m_renderDevice.getUUID(), { window->getUUID() });

		PipelineConstructInfo info = { m_renderSwapchain.getUUID(), m_descriptor.getUUID(), m_shaders.getUUID() };
		m_renderPipeline.init(m_renderDevice.getUUID(), info);

		m_renderSwapchain.generateFramebuffers(m_renderPipeline.getRenderPass());

		m_window = window;
	}

	void RenderContext::Load(std::shared_ptr<Uniform>& buffer)
	{
		buffer->init(m_renderDevice.getUUID());
		m_ub = buffer;
	}

	void RenderContext::Load(std::shared_ptr<TextureImage>& img)
	{
		img->init(m_renderDevice.getUUID());
		m_tx = img;
	}

	void RenderContext::Load(std::shared_ptr<Buffer>& buffer)
	{
		buffer->init(m_renderDevice.getUUID());
		m_buffer = buffer;
	}

	void RenderContext::Load(std::shared_ptr<Canvas>& canvas)
	{
		if (auto window = m_window.lock()) {
			CanvasConstructInfo info{
				window->getUUID(),
				m_renderPipeline.getUUID(),
				m_renderSwapchain.getUUID(),
				m_descriptor.getUUID()
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
		m_renderDevice.createDependencies({ (int)m_renderSwapchain.getImageCount() });

		uint64_t ub = 0;
		uint64_t tx = 0;

		if (auto ubRef = m_ub.lock()) ub = ubRef->getUUID();
		if (auto txRef = m_tx.lock()) tx = txRef->getUUID();
		m_descriptor.createDescriptorSets(ub, tx);

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
		m_renderSwapchain.generateFramebuffers(m_renderPipeline.getRenderPass());
	}

	void RenderContext::Draw()
	{
		if (!m_state.isInitialized) {
			Alert("Render Context not fully initialized before drawing!", FATAL);
			return;
		}
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

		DrawInfo drawInfo = {
			m_renderPipeline,
			m_renderSwapchain,
			m_descriptor,
			m_ub,
			m_buffer,
			m_cnvs
		};
		
		m_renderDevice.draw(drawInfo);
	}

	void RenderContext::Destroy()
	{
		m_shaders.destroy();
		m_descriptor.destroy();
		m_renderSwapchain.destroy();
		m_renderPipeline.destroy();

		m_renderDevice.destroy();
	}
}