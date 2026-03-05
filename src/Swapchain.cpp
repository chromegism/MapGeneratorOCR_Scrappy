#include "pch.h"

#include "Swapchain.h"
#include "DEBUG_LOG.h"

VkSurfaceFormatKHR Swapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
	for (const auto& format : formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return formats[0];
}

VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
	for (const auto& mode : modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			DEBUG_LOG << "Mailbox mode selected (triple buffering)" << std::endl;
			return mode;
		}
	}

	DEBUG_LOG << "Fifo mode selected (double buffering)" << std::endl;

	return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t Swapchain::chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) {
	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}
	return imageCount;
}

VkExtent2D Swapchain::chooseExtent(SDL_Window* window, const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		SDL_GetWindowSizeInPixels(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

Swapchain::Swapchain(const SwapchainCreateInfos& info) : device_(&info.device) {
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = info.surface;
	createInfo.minImageCount = info.imageCount;
	createInfo.imageFormat = info.surfaceFormat.format;
	createInfo.imageColorSpace = info.surfaceFormat.colorSpace;
	createInfo.imageExtent = info.extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { info.device.graphicsQueue().familyIndex(), info.device.presentQueue().familyIndex() };

	if (info.device.graphicsQueue().familyIndex() != info.device.presentQueue().familyIndex()) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = info.transform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = info.presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(info.device.handle(), &createInfo, nullptr, &handle_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	} DEBUG_ELSE{
		std::cout << "Successfully created swapchain" << std::endl;
	}

	surface_ = info.surface;
	extent_ = info.extent;
	surfaceFormat_ = info.surfaceFormat;
	window_ = info.window;

	genImages(info.imageCount);
	genRenderPass();
	genFramebuffers();
}

void Swapchain::genImages(uint32_t imageCount) {
	vkGetSwapchainImagesKHR(device_->handle(), handle_, &imageCount, nullptr);
	images_.reserve(imageCount);

	std::vector<VkImage> swapchainImagehandles(imageCount);
	vkGetSwapchainImagesKHR(device_->handle(), handle_, &imageCount, swapchainImagehandles.data());
	for (const auto& handle : swapchainImagehandles) {
		images_.emplace_back(SwapchainImage::fromImage(device_->handle(), handle, surfaceFormat_.format));
	}

	depthImage_ = Image::createDepth(*device_, extent_.width, extent_.height, Image::findDepthFormat(device_->physicalDevice().handle()));
}

void Swapchain::genRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = surfaceFormat_.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = Image::findDepthFormat(device_->physicalDevice().handle());
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult error_code = vkCreateRenderPass(device_->handle(), &renderPassInfo, nullptr, &renderPass_);
	handleVkResult(error_code, "Failed to create render pass");
}

void Swapchain::genFramebuffers() {
	framebuffers_.resize(images_.size());

	for (size_t i = 0; i < images_.size(); i++) {
		framebuffers_.at(i) = Framebuffer::createDefaultSwap(
			device_->handle(), images_.at(i).view(),
			depthImage_.view(), renderPass_,
			extent_.width, extent_.height
		);
	}
}