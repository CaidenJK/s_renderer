#include "SwapChain.h"

#include <vulkan/vk_enum_string_helper.h>
#include <algorithm>

#include "Device.h"

namespace Render 
{
	SwapChain::SwapChain() 
	{
	}

	SwapChain::~SwapChain() 
	{
		destroy();
	}

	void SwapChain::init(size_t deviceUUID, SwapChainConstructInfo info)
	{
		device = Request<Device>(deviceUUID, "self");
		window = Request<Window>(info.windowUUID, "self");

		constructSwapChain();
		createSyncObjects();
	}

	void SwapChain::destroy()
	{
		cleanupSwapChain();

		if (device) {
			for (auto imageAvailableSemaphore : m_imageAvailableSemaphores) {
				vkDestroySemaphore((*device).getDevice(), imageAvailableSemaphore, nullptr);
			}
			m_imageAvailableSemaphores.clear();
			for (auto renderFinishedSemaphore : m_renderFinishedSemaphores) {
				vkDestroySemaphore((*device).getDevice(), renderFinishedSemaphore, nullptr);
			}
			m_renderFinishedSemaphores.clear();
			for (auto inFlightFence : m_inFlightFences) {
				vkDestroyFence((*device).getDevice(), inFlightFence, nullptr);
			}
			m_inFlightFences.clear();
		}
	}

	void SwapChain::needRecreate()
	{
		recreate = true;
		(*window).resetFramebufferResizedFlag();
	}
	
	void SwapChain::constructSwapChain()
	{
		if (device.wait() != Manager::State::YES || window.wait() != Manager::State::YES) {
			Alert("Swap chain could not be created due to death of required resources.", FATAL);
		}
		auto pd = (*device).getPhysicalDevice();
		auto surface = (*device).getSurface();

		auto supportDetails = querySwapChainSupport(pd, surface);

		cleanupSwapChain();
		createSwapChain(supportDetails, (*device).getQueueFamilies(), (*device).getSurface());

		recreate = false;
	}

	void SwapChain::createSwapChain(SwapChainSupportDetails& swapChainSupport, QueueFamilyIndices& indices, VkSurfaceKHR& surface) 
	{
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		// Ignore alpha
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = nullptr;

		if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used.", FATAL);
			return;
		}
		if (vkCreateSwapchainKHR((*device).getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			Alert("Failed to create swap chain!", FATAL);
			return;
		}

		vkGetSwapchainImagesKHR((*device).getDevice(), swapChain, &imageCount, nullptr);
		swapChainImageBuffers.resize(imageCount);

		std::vector<VkImage> swapChainImages; swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR((*device).getDevice(), swapChain, &imageCount, swapChainImages.data());

		imageFormats[0] = surfaceFormat.format;
		swapChainExtent = extent;

		for (int i = 0; i < swapChainImages.size(); i++) {
			swapChainImageBuffers[i].init((*device).getUUID());
			swapChainImageBuffers[i].setImage(swapChainImages[i], false);
			swapChainImageBuffers[i].createImageView(imageFormats[0], VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		imageFormats[1] = swapChainSupport.depthBufferFormat;

		msaaSamples = (*device).getConfig().desiredMSAASamples;

		createColorResources();
		createDepthResources();
	}

	VkFormat SwapChain::findSupportedFormat(VkPhysicalDevice& device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
	{
		for (VkFormat format : candidates) {
    		VkFormatProperties props;
    		vkGetPhysicalDeviceFormatProperties(device, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		return VK_FORMAT_UNDEFINED;
	}

	bool SwapChain::hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkFormat SwapChain::findDepthFormat(VkPhysicalDevice& device) {
		return findSupportedFormat(device,
			{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	void SwapChain::createDepthResources()
	{
		if (!depthBuffer) depthBuffer = std::make_shared<ImageBuffer>();
		depthBuffer->init((*device).getUUID());

		depthBuffer->createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, imageFormats[1], VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		depthBuffer->createImageView(imageFormats[1], VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	}

	void SwapChain::createColorResources()
	{
		if (!colorBuffer) colorBuffer = std::make_shared<ImageBuffer>();
		colorBuffer->init((*device).getUUID());

		colorBuffer->createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, imageFormats[0], VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
		colorBuffer->createImageView(imageFormats[0], VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}

	void SwapChain::generateFramebuffers(VkRenderPass& renderPass)
	{
		if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used.", FATAL);
			return;
		}
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer((*device).getDevice(), framebuffer, nullptr);
		}

		swapChainFramebuffers.resize(swapChainImageBuffers.size());

		for (size_t i = 0; i < swapChainImageBuffers.size(); i++) {
			std::array<VkImageView, 3> attachments = {
				colorBuffer->getImageView(),
				depthBuffer->getImageView(),
				swapChainImageBuffers[i].getImageView()
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer((*device).getDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				Alert("Failed to create a required framebuffers!", FATAL);
				return;
			}
		}
	}

	void SwapChain::cleanupSwapChain()
	{
		if (device) {
			for (auto& ib : swapChainImageBuffers) {
				vkDestroyImageView((*device).getDevice(), ib.getImageView(), nullptr);
				ib.getImageView() = VK_NULL_HANDLE;
			}
			swapChainImageBuffers.clear(); swapChainImageBuffers = {};
		}
		
		if (depthBuffer) depthBuffer->destroy();
		if (colorBuffer) colorBuffer->destroy();
		
		if (device) {
			for (auto framebuffer : swapChainFramebuffers) {
				vkDestroyFramebuffer((*device).getDevice(), framebuffer, nullptr);
			}
		}
		swapChainFramebuffers.clear();
		if (device && swapChain) {
			vkDestroySwapchainKHR((*device).getDevice(), swapChain, nullptr);
			swapChain = VK_NULL_HANDLE;
		}
	}

	void SwapChain::createSyncObjects()
	{
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(getImageCount());
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore((*device).getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence((*device).getDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {

				Alert("Failed to create synchronization objects for all frames!", FATAL);
				return;
			}
		}
		for (size_t i = 0; i < getImageCount(); i++) {
			if (vkCreateSemaphore((*device).getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
				Alert("Failed to create synchronization objects for all frames!", FATAL);
				return;
			}
		}
	}

	void SwapChain::aquireNextImage(uint32_t currentFrame)
	{
		if (m_imageAvailableSemaphores.size() == 0 ||
			m_renderFinishedSemaphores.size() == 0 ||
			m_inFlightFences.size() == 0) {
			Alert("Cannot aquire image from uninitilized swapchain.", FATAL);
			return;
		}

		vkWaitForFences((*device).getDevice(), 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		// Aquire image from swapchain
		VkResult result = vkAcquireNextImageKHR((*device).getDevice(), swapChain, UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &swapChainImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			shouldRecreate();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			Alert("Failed to acquire swap chain image!", FATAL);
			return;
		}
	}

	void SwapChain::submitCommandBuffer(VkCommandBuffer& commandBuffer, uint32_t currentFrame)
	{
		vkResetFences((*device).getDevice(), 1, &m_inFlightFences[currentFrame]);

		//vkResetCommandBuffer(commandBuffers[currentFrame], 0);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[swapChainImageIndex]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit((*device).getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS) {
			Alert("Failed to submit draw command buffer!", FATAL);
			return;
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &swapChainImageIndex;
		presentInfo.pResults = nullptr; // Optional

		auto result = vkQueuePresentKHR((*device).getPresentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			shouldRecreate();
			return;
		}
		else if (result != VK_SUCCESS) {
			Alert("Failed to acquire swap chain image!", FATAL);
			return;
		}
	}

	SwapChain::SwapChainSupportDetails SwapChain::querySwapChainSupport(VkPhysicalDevice& device, VkSurfaceKHR surface) 
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		details.depthBufferFormat = findDepthFormat(device);

		return details;
	}

	VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
	{
		VkSurfaceFormatKHR currentSwapSurfaceFormat;
		bool isSet = false;
		for (const auto& availableFormat : availableFormats) {
			// UNORM
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				currentSwapSurfaceFormat = availableFormat;
				isSet = true;
				break;
			}
		}
		if (!isSet) {
			currentSwapSurfaceFormat = availableFormats[0];
		}
		if (swapChainExtent.height == 0 || swapChainExtent.width == 0) {
			Alert("Chosen Swap Surface Format: " + std::string(string_VkFormat(currentSwapSurfaceFormat.format)) + ", Color Space: " + std::string(string_VkColorSpaceKHR(currentSwapSurfaceFormat.colorSpace)) + "\n", INFO);
		}
		return currentSwapSurfaceFormat;
	}

	VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
	{
		VkPresentModeKHR currentPresentMode;
		bool isSet = false;
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				currentPresentMode = availablePresentMode;
				isSet = true;
				break;
			}
		}

		if (!isSet) {
			currentPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		}

		if (swapChainExtent.height == 0 || swapChainExtent.width == 0) {
			Alert("Chosen Present Mode: " + std::string(string_VkPresentModeKHR(currentPresentMode)) + "\n", INFO);
		}
		return currentPresentMode;
	}

	VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			(*window).getFramebufferSize(width, height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}
}