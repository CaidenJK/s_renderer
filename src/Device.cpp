#include "Device.h"
#include <vulkan/vk_enum_string_helper.h>

#include <map>
#include <set>
#include <cstdint>
#include <string>
#include <cstring>

namespace Render 
{
	VulkanDebugger* Device::debugger = nullptr;
	void VulkanDebugger::registerDebugAlert(const std::string& message, CallSeverity severity) {
		Alert(message, severity);
	}

	// Debug messenger proxy functions
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	void Device::debugMessengerCreateInfoFactory(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
	}

	Device::Device()
	{
#ifndef NDEBUG
		m_enableValidationLayers = true;
		debugger = new VulkanDebugger();
#else
		debugger = nullptr;
		m_enableValidationLayers = false;
#endif
	}

	Device::~Device() 
	{
		destroy();
		delete debugger;
	}

	void Device::init(DeviceConfig config)
	{
		m_config = config;
		initVulkan();
	}

	void Device::destroy()
	{
		if (m_instance) {
			if (m_commandPool != VK_NULL_HANDLE) {
				vkDestroyCommandPool(m_device, m_commandPool, nullptr);
			}

			for (auto imageAvailableSemaphore : m_imageAvailableSemaphores) {
				vkDestroySemaphore(m_device, imageAvailableSemaphore, nullptr);
			}
			for (auto renderFinishedSemaphore : m_renderFinishedSemaphores) {
				vkDestroySemaphore(m_device, renderFinishedSemaphore, nullptr);
			}
			for (auto inFlightFence : m_inFlightFences) {
				vkDestroyFence(m_device, inFlightFence, nullptr);
			}

			vkDestroyDevice(m_device, nullptr); // ---- DEVICE DESTRUCTION ----

			if (m_enableValidationLayers) {
				DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
			}
			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

			vkDestroyInstance(m_instance, nullptr); // ---- INSTANCE DESTRUCTION ----
			m_instance = VK_NULL_HANDLE;

			for (auto& ask : asks) {
				ask->invalidate();
			}
		}
	}

	void Device::fillImGuiInfo(ImGui_ImplVulkan_InitInfo* info)
	{
		info->Instance = m_instance;
		info->PhysicalDevice = m_physicalDevice;
		info->Device = m_device;
		info->QueueFamily = m_queueFamilyIndices.presentFamily.value();
		info->Queue = m_presentQueue;
		info->PipelineCache = nullptr;
		info->DescriptorPool = nullptr;
		info->MinImageCount = 2;
		info->ImageCount = 0;
		info->Allocator = nullptr;
		info->PipelineInfoMain.RenderPass = nullptr;
		info->PipelineInfoMain.Subpass = 0;
		info->PipelineInfoMain.MSAASamples = m_config.desiredMSAASamples;
		info->CheckVkResultFn = nullptr;
	}

	void Device::initVulkan() 
	{
		checkValidationLayerSupport();

		createInstance();

		setupDebugMessenger();
		checkVKExtensions();

		createSurface();

		pickPhysicalDevice();
		createLogicalDevice();

		createCommmandPool();
		createCommandBuffers();
	}

	void Device::setupDebugMessenger() 
	{
		if (!m_enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		debugMessengerCreateInfoFactory(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
			Alert("Could not setup debug messenger!", FATAL);
			return;
		}
	}

	void Device::checkValidationLayerSupport() 
	{
		if (!m_enableValidationLayers) return;

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : m_validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				Alert("Validation layer Requested not available!", FATAL);
				return;
			}
		}
	}

	void Device::checkVKExtensions() 
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		m_vkExtensions.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_vkExtensions.data());

		std::string message = "Avalible Vulkan Extensions: \n";
		if (extensionCount == 0) {
			message += "\tNo extensions found.";
		}
		else {
			for (const auto& extension : m_vkExtensions) {
				message += '\t' + std::string(extension.extensionName) + '\n';
			}
		}
		Alert(message, INFO);
	}

	std::vector<const char*> Device::getRequiredGLFWExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (m_enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void Device::createSurface()
	{
		if (auto wndw = m_config.window.lock()) {
			wndw->createVulkanSurface(m_instance, m_surface);
		}
		else {
			Alert("Window reference is NULL.", FATAL);
		}
	}

	void Device::createInstance() 
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Starry";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
#ifdef __APPLE__
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

		if (m_enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
			createInfo.ppEnabledLayerNames = m_validationLayers.data();

			const VkBool32 verbose_value = true;
			const VkLayerSettingEXT layerSetting = {"VK_LAYER_KHRONOS_validation", "validate_best_practices", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &verbose_value};
			VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr, 1, &layerSetting};

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
			debugMessengerCreateInfoFactory(debugCreateInfo);
			debugCreateInfo.pNext = &layerSettingsCreateInfo;
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		auto extensions = getRequiredGLFWExtensions();
		for (const auto& exten : m_instanceExtensions) {
			extensions.emplace_back(exten);
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		

		if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
			Alert("Vulkan instance creation failed!", FATAL);
			return;
		}
	}

	bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		if (!requiredExtensions.empty()) {
			std::string message = "Device missing extensions: \n";
			for (const auto& ext : requiredExtensions) {
				message += '\t' + ext + '\n';
			}
			Alert(message, WARNING);
			return false;
		}
		return true;
	}

	QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		// Find a queue that supports VK_QUEUE_GRAPHICS_BIT
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			if (indices.isComplete()) { break; }
			i++;
		}

		return indices;
	}

	Device::DeviceInfo Device::isDeviceSuitable(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		DeviceInfo info;
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		info.score = 1;
		std::strncpy(info.name, deviceProperties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			info.score += 1000;
		}

		// Texture max size
		info.score += deviceProperties.limits.maxImageDimension2D;

		bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChain::SwapChainSupportDetails swapChainSupport = SwapChain::querySwapChainSupport(physicalDevice, m_surface);
			// At least one of each
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

		info.isSuitible = indices.isComplete() && extensionsSupported && swapChainAdequate && info.score > 0 &&
			supportedFeatures.samplerAnisotropy;
		return info;
	}

	VkSampleCountFlagBits Device::getMaxUsableSampleCount() {
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		VkSampleCountFlagBits result;

		if (counts & VK_SAMPLE_COUNT_64_BIT) { result = VK_SAMPLE_COUNT_64_BIT; }
		else if (counts & VK_SAMPLE_COUNT_32_BIT) { result = VK_SAMPLE_COUNT_32_BIT; }
		else if (counts & VK_SAMPLE_COUNT_16_BIT) { result = VK_SAMPLE_COUNT_16_BIT; }
		else if (counts & VK_SAMPLE_COUNT_8_BIT) { result = VK_SAMPLE_COUNT_8_BIT; }
		else if (counts & VK_SAMPLE_COUNT_4_BIT) { result = VK_SAMPLE_COUNT_4_BIT; }
		else if (counts & VK_SAMPLE_COUNT_2_BIT) { result = VK_SAMPLE_COUNT_2_BIT; }
		else { result = VK_SAMPLE_COUNT_1_BIT; }

		return std::min(result, m_config.desiredMSAASamples);
	}

	void Device::pickPhysicalDevice() 
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			Alert("Failed to find GPUs with Vulkan support!", FATAL);
			return;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		std::string messsage = "Available Vulkan Devices: \n";
		std::multimap<int, VkPhysicalDevice> candidates;
		for (const auto& device : devices) {
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			DeviceInfo deviceInfo = isDeviceSuitable(device);

			if (deviceInfo.isSuitible) {
				messsage += "\t" + std::string(deviceInfo.name) + ", score: " + std::to_string(deviceInfo.score) + "\n";
				candidates.insert(std::make_pair(deviceInfo.score, device));
			}
		}

		if (candidates.empty() || candidates.rbegin()->first < 1) {
			Alert("Failed to find a suitable GPU!", FATAL);
			return;
		}
		Alert(messsage, INFO);
		m_physicalDevice = candidates.rbegin()->second;
		m_config.desiredMSAASamples = getMaxUsableSampleCount();
		m_queueFamilyIndices = findQueueFamilies(m_physicalDevice);
	}

	void Device::createLogicalDevice() 
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { m_queueFamilyIndices.graphicsFamily.value(), m_queueFamilyIndices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.sampleRateShading = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

		if (m_enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
			createInfo.ppEnabledLayerNames = m_validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		// No device specific extensions needed yet

		if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
			Alert("Failed to create logical device!", FATAL);
			return;
		}

		vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
	}

	void Device::createCommmandPool()
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
			Alert("Failed to create command pool!", FATAL);
			return;
		}
	}

	void Device::createCommandBuffers()
	{
		m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();;

		if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
			Alert("Failed to allocate command buffers!", FATAL);
			return;
		}
	}

	void Device::createSyncObjects(DependencyConfig& config)
	{
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(config.imageCount);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {

				Alert("Failed to create synchronization objects for all frames!", FATAL);
				return;
			}
		}
		for (size_t i = 0; i < config.imageCount; i++) {
			if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
				Alert("Failed to create synchronization objects for all frames!", FATAL);
				return;
			}
		}
	}

	void Device::createDependencies(DependencyConfig config)
	{
		if (m_commandPool == VK_NULL_HANDLE) {
			createCommmandPool();
			createCommandBuffers();
		}
		for (auto imageAvailableSemaphore : m_imageAvailableSemaphores) {
			vkDestroySemaphore(m_device, imageAvailableSemaphore, nullptr);
		}
		m_imageAvailableSemaphores.clear();
		for (auto renderFinishedSemaphore : m_renderFinishedSemaphores) {
			vkDestroySemaphore(m_device, renderFinishedSemaphore, nullptr);
		}
		m_renderFinishedSemaphores.clear();
		for (auto inFlightFence : m_inFlightFences) {
			vkDestroyFence(m_device, inFlightFence, nullptr);
		}
		m_inFlightFences.clear();

		createSyncObjects(config);
	}

	// For each command buffer
	// Render Pass
	// Swap Chain FrameBuffers
	// Swap Chain Extent
	// Clear Values
	// Vertex Buffers -> VkBuffer vertex, VkBuffer index
	// update Uniform Buffers
	// bind Uniform Buffer Descriptor Sets with Pipeline Layout

	/* input struct ->
		descriptors
		buffer - one for now, join on ready
		swapChain
		renderPass
	<- */ 
	void Device::recordCommandBuffer(DrawInfo& info, VkCommandBuffer commandBuffer, uint32_t imageIndex) 
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			Alert("Failed to begin recording command buffer", FATAL);
			return;
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = info.pipeline.getRenderPass();
		renderPassInfo.framebuffer = info.swapChain.getFramebuffers()[imageIndex];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = info.swapChain.getExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {m_config.clearColor.x, m_config.clearColor.y, m_config.clearColor.z, 1.0f} };
		clearValues[1].depthStencil = {1.0f, 0};

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Start of recording
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline.getPipeline());
		
		uint32_t numberOfIndices = 0;
		if (auto vb = info.Buffer.lock()) {
			VkBuffer buffers[] = { vb->getBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

			vkCmdBindIndexBuffer(commandBuffer, vb->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
			numberOfIndices = static_cast<uint32_t>(vb->getNumIndices());
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline.getPipelineLayout(), 0, 1, &(info.descriptor.getDescriptorSet(m_currentFrame)), 0, nullptr);
		
		if (auto ub = info.Uniform.lock()) {
			ub->updateUniform(m_currentFrame);
		}

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(info.swapChain.getExtent().width);
		viewport.height = static_cast<float>(info.swapChain.getExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = info.swapChain.getExtent();
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdDrawIndexed(commandBuffer, numberOfIndices, 1, 0, 0, 0);

		if (auto canvas = info.canvas.lock()) {
			canvas->Record(commandBuffer);
		}

		// End
		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			Alert("Failed to record command buffer!", FATAL);
			return;
		}
	}

	// Draw Frame
	// Swap Chain
	// If VK_ERROR_OUT_OF_DATE_KHR, recreate swap chain
	// If window resized, skip

	void Device::draw(DrawInfo info)
	{
		ERROR_VOLATILE();
		if (m_commandPool == VK_NULL_HANDLE ||
			m_imageAvailableSemaphores.size() == 0 ||
			m_renderFinishedSemaphores.size() == 0 ||
			m_inFlightFences.size() == 0) {
			Alert("Draw called before Device was fully initilized.", FATAL);
			return;
		}

		vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

		// Aquire image from swapchain
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_device, info.swapChain.getSwapChain(), UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			info.swapChain.shouldRecreate();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			Alert("Failed to acquire swap chain image!", FATAL);
			return;
		}
		
		recordCommandBuffer(info, m_commandBuffers[m_currentFrame], imageIndex);
		vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

		//vkResetCommandBuffer(commandBuffers[currentFrame], 0);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[imageIndex]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
			Alert("Failed to submit draw command buffer!", FATAL);
			return;
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { info.swapChain.getSwapChain() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			info.swapChain.shouldRecreate();
			return;
		}
		else if (result != VK_SUCCESS) {
			Alert("Failed to acquire swap chain image!", FATAL);
			return;
		}

		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Device::waitIdle() 
	{
		vkDeviceWaitIdle(m_device);
	}

	void Device::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		if (buffer != VK_NULL_HANDLE || bufferMemory != VK_NULL_HANDLE) {
			Alert("Vertex buffer already created! All calls other than the first are skipped.", WARNING);
			return;
		}

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			Alert("Failed to create buffer!", FATAL);
			return;
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;

		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		auto state = vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory);

		if (state != VK_SUCCESS) {
			Alert("Failed to allocate buffer memory!", FATAL);
			return;
		}

		vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
	}

	uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		if (m_physicalDevice == VK_NULL_HANDLE) {
			Alert("Vulkan physical device null! Can't find memory type.", FATAL);
			return 0;
		}
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		Alert("Failed to find suitable memory type on given device!", FATAL);
		return 0;
	}

	void Device::copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	VkCommandBuffer Device::beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void Device::endSingleTimeCommands(VkCommandBuffer& commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_graphicsQueue);

		vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Device::debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (debugger == nullptr) return VK_FALSE;

		CallSeverity severity = INFO;
		if (messageSeverity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)) {
				severity = INFO_URGANT;
			}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			severity = WARNING;
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			severity = CRITICAL;
		}

		debugger->registerDebugAlert("Validation layer: " + std::string(pCallbackData->pMessage) + "\n", severity);
		return VK_FALSE;
	}
}