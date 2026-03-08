#include "pch.h"

#include "Render.h"
#include "DEBUG_LOG.h"

std::vector<char> readFileCharVector(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file) {
        throw std::runtime_error("failed to open file " + filename);
    }

	size_t filesize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(filesize);

	file.seekg(0);
	file.read(buffer.data(), filesize);

	file.close();

	return buffer;
}

void Renderer::init(SDL_Window* window, TerrainGenerator& generator) {
	createInstance();
	surface = Surface(instance.handle(), window);
	physicalDevice = PhysicalDevice::pickBest(instance, surface);
	device = LogicalDevice(physicalDevice);

	PhysicalDevice::SwapChainSupportDetails swapchainSupport = physicalDevice.swapChainSupportDetails();
	uint32_t maxFramesInFlight = Swapchain::chooseImageCount(swapchainSupport.capabilities);
	swapchain = Swapchain::create(
		device, surface.handle(), window,
		Swapchain::chooseSurfaceFormat(swapchainSupport.formats),
		Swapchain::choosePresentMode(swapchainSupport.presentModes),
		Swapchain::chooseExtent(window, swapchainSupport.capabilities),
		swapchainSupport.capabilities.currentTransform,
		maxFramesInFlight
	);
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createErosionDescriptorSetLayout();
	createErosionPipeline();
	createCommandPool();
	createVertexBuffer(generator);
	createIndexBuffer(generator);
	createHeightImage(generator);
	createErosionImages(generator);
	createHeightSampler();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createErosionDescriptorSets();
	createCommandBuffers();
	createErosionFence();
	isFirstFrame = true;
}

void Renderer::createInstance() {
	std::vector<std::string> extensions = Instance::requiredExtensions();

	std::vector<std::string> additionalExtensions{ VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
	for (const auto& ext : additionalExtensions) {
		extensions.emplace_back(ext);
	}

	// Optional: Enable validation layers (for debugging)
	std::vector<std::string> layers = {};
	DEBUG_RUN{
		layers.emplace_back("VK_LAYER_KHRONOS_validation");
		std::cout << "Validation layers enabled" << std::endl;
	}

	instance = Instance("Render Test", {1, 0, 0}, "No engine", {1, 0, 0}, VK_API_VERSION_1_3, std::move(extensions), std::move(layers));

	DEBUG_LOG << "Instance created successfully" << std::endl;
}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device.handle(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module");
	}

	return shaderModule;
}

void Renderer::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding mvpBinding{};
	mvpBinding.binding = 0;
	mvpBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	mvpBinding.descriptorCount = 1;
	mvpBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	mvpBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding mapBinding{};
	mapBinding.binding = 1;
	mapBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	mapBinding.descriptorCount = 1;
	mapBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	mapBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding heightSamplerLayoutBinding{};
	heightSamplerLayoutBinding.binding = 2;
	heightSamplerLayoutBinding.descriptorCount = 1;
	heightSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	heightSamplerLayoutBinding.pImmutableSamplers = nullptr;
	heightSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { mvpBinding, mapBinding, heightSamplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device.handle(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
}

void Renderer::createGraphicsPipeline() {
	auto vertShaderCode = readFileCharVector("shaders/test.vert.spv");
	auto fragShaderCode = readFileCharVector("shaders/test.frag.spv");

	DEBUG_LOG << "Read shader modules" << std::endl;

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	DEBUG_LOG << "Created shader modules" << std::endl;

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();


	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescriptions = Vertex::getBindingDescriptions();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	inputAssembly.primitiveRestartEnable = VK_FALSE;


	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapchain.extent().width;
	viewport.height = (float)swapchain.extent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchain.extent();


	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;


	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	VkResult error_code = vkCreatePipelineLayout(device.handle(), &pipelineLayoutInfo, nullptr, &pipelineLayout);
	handleVkResult(error_code, "Failed to create pipeline layout");


	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = swapchain.renderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	error_code = vkCreateGraphicsPipelines(device.handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
	handleVkResult(error_code, "Failed to create graphics pipeline");


	vkDestroyShaderModule(device.handle(), vertShaderModule, nullptr);
	vkDestroyShaderModule(device.handle(), fragShaderModule, nullptr);

	DEBUG_LOG << "Successfully created graphics pipeline" << std::endl;
}


void Renderer::createErosionDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding inputBinding{};
	inputBinding.binding = 0;
	inputBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	inputBinding.descriptorCount = 1;
	inputBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	inputBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding outputBinding{};
	outputBinding.binding = 1;
	outputBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	outputBinding.descriptorCount = 1;
	outputBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	outputBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { inputBinding, outputBinding };
	createInfo.pBindings = bindings.data();
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	
	VkResult error_code = vkCreateDescriptorSetLayout(device.handle(), &createInfo, nullptr, &erosionDescriptorSetLayout);
}


void Renderer::createErosionPipeline() {
	auto compShaderCode = readFileCharVector("shaders/test.comp.spv");
	VkShaderModule compShaderModule = createShaderModule(compShaderCode);

	VkPipelineShaderStageCreateInfo compStageInfo{};
	compStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compStageInfo.module = compShaderModule;
	compStageInfo.pName = "main";

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &erosionDescriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 0;

	VkResult error_code = vkCreatePipelineLayout(device.handle(), &layoutInfo, nullptr, &erosionPipelineLayout);
	handleVkResult(error_code, "Failed to create erosion pipeline layout");

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = erosionPipelineLayout;
	pipelineInfo.stage = compStageInfo;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	error_code = vkCreateComputePipelines(device.handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &erosionPipeline);
	handleVkResult(error_code, "Failed to create erosion compute pipeline");

	vkDestroyShaderModule(device.handle(), compShaderModule, nullptr);
}


void Renderer::createCommandPool() {
	PhysicalDevice::QueueFamilyIndices queueFamilyIndices = physicalDevice.queueFamilyIndices();

	VkCommandPoolCreateInfo pool1Info{};
	pool1Info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool1Info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool1Info.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	VkResult error_code = vkCreateCommandPool(device.handle(), &pool1Info, nullptr, &commandPool);
	handleVkResult(error_code, "Failed to create command pool");

	VkCommandPoolCreateInfo pool2Info{};
	pool2Info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool2Info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool2Info.queueFamilyIndex = queueFamilyIndices.computeFamily.value();

	error_code = vkCreateCommandPool(device.handle(), &pool2Info, nullptr, &erosionCommandPool);
	handleVkResult(error_code, "Failed to create command pool");

	DEBUG_LOG << "Successfully created command pool" << std::endl;
}

void Renderer::createVertexBuffer(TerrainGenerator& generator) {
	uint32_t bufferLength = generator.details.width * generator.details.height;
	uint32_t bufferSize = sizeof(glm::vec2) * bufferLength;

	Buffer stagingBuffer = Buffer::createStaging(device, bufferSize);

	glm::vec2* data = stagingBuffer.mapMemory<glm::vec2>();
	generator.genVertexChunksInto(data);
	stagingBuffer.unmapMemory();

	vertexBuffer = Buffer::createVertex(device, bufferSize);

	VkCommandBuffer commandBuffer = beginSingleCommand(device.handle(), commandPool);
	vertexBuffer.copyBuffer(stagingBuffer, commandBuffer);
	endCommand(commandBuffer);
	device.graphicsSubmitCommand(commandPool, commandBuffer);
	//endAndSubmitCommand(device.handle(), commandPool, device.graphicsQueue().handle(), commandBuffer);

	DEBUG_LOG << "Successfully created vertex buffer" << std::endl;
}


void Renderer::createIndexBuffer(TerrainGenerator& generator) {
	VkDeviceSize terrainIndicesLength = generator.calcIndicesLength();
	VkDeviceSize bufferSize = sizeof(uint32_t) * terrainIndicesLength;

	Buffer stagingBuffer = Buffer::createStaging(device, bufferSize);

	uint32_t* data = stagingBuffer.mapMemory<uint32_t>();
	generator.genTriangleIndicesInto(data);
	stagingBuffer.unmapMemory();

	indexBuffer = Buffer::createIndex(device, bufferSize);
	VkCommandBuffer commandBuffer = beginSingleCommand(device.handle(), commandPool);
	indexBuffer.copyBuffer(stagingBuffer, commandBuffer);
	endCommand(commandBuffer);
	device.graphicsSubmitCommand(commandPool, commandBuffer);
}

void Renderer::createErosionImages(TerrainGenerator& generator) {
	erosionImageStager = Buffer::createStaging(device, sizeof(float) * generator.details.width * generator.details.height);
	erosionImages[0] = Image::createStorage(device, generator.details.width, generator.details.height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	erosionImages[1] = Image::createStorage(device, generator.details.width, generator.details.height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	VkCommandBuffer commandBuffer = beginSingleCommand(device.handle(), commandPool);

	erosionImages[0].transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, commandBuffer, true);
	erosionImages[1].transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, commandBuffer, false);

	device.computeSubmitCommand(commandPool, commandBuffer);

	erosionImageStagerMapped = erosionImageStager.mapMemory<float>();

	updateCurrentErosionImage(generator);
}


void Renderer::updateCurrentErosionImage(TerrainGenerator& generator) {
	mapDetailsData.bufferSize = glm::ivec2(generator.details.width, generator.details.height);
	mapDetailsData.displaySize = glm::vec2(4, 4);

	uint32_t bufferLength = generator.details.width * generator.details.height;
	uint32_t bufferSize = sizeof(float) * bufferLength;

	if (bufferSize != erosionImageStager.size()) {
		erosionImageStager = Buffer::createStaging(device, bufferSize);
		erosionImageStagerMapped = erosionImageStager.mapMemory<float>();
	}

	generator.genTerrainInto(erosionImageStagerMapped);

	VkCommandBuffer commandBuffer = beginSingleCommand(device.handle(), commandPool);
	erosionImages[imageIndex].copyBuffer(erosionImageStager, commandBuffer);
	renderHeightImage.copyImage(erosionImages[imageIndex], commandBuffer);
	endCommand(commandBuffer);
	device.graphicsSubmitCommand(commandPool, commandBuffer);
}


void Renderer::createHeightImage(TerrainGenerator& generator) {
	renderHeightImage = Image::createStorage(device, generator.details.width, generator.details.height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, true);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult error_code = vkCreateSemaphore(device.handle(), &semaphoreInfo, nullptr, &copyCompleteSemaphore);
	handleVkResult(error_code, "Failed to create fence");
	error_code = vkCreateSemaphore(device.handle(), &semaphoreInfo, nullptr, &renderCompleteSemaphore);
	handleVkResult(error_code, "Failed to create fence");

	DEBUG_LOG << "Successfully created height image" << std::endl;
}


void Renderer::createHeightSampler() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.f;

	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device.handle(), &samplerInfo, nullptr, &renderHeightSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}

	DEBUG_LOG << "Successfully created height image sampler" << std::endl;
}

void Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device.handle(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device.handle(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device.handle(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device.handle(), buffer, bufferMemory, 0);
}

void Renderer::createUniformBuffers() {
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(physicalDevice.handle(), &props);

	size_t alignment = props.limits.minUniformBufferOffsetAlignment;

	auto alignedSize = [](size_t size, size_t alignment) {
		return (size + alignment - 1) & ~(alignment - 1);
		};

	size_t mvpSize = alignedSize(sizeof(MVPBufferObject), alignment);
	size_t mapSize = alignedSize(sizeof(MapDetailsObject), alignment);

	VkDeviceSize totalSize = mvpSize + mapSize;

	uniformBuffers.reserve(swapchain.maxFramesInFlight());
	uniformBuffersMapped.resize(swapchain.maxFramesInFlight());

	for (size_t i = 0; i < swapchain.maxFramesInFlight(); i++) {
		uniformBuffers.emplace_back(Buffer::createUniform(device, totalSize));
		uniformBuffersMapped[i] = uniformBuffers[i].mapMemory<void>();
	}
}

void Renderer::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 3> poolSizes{};
	// Heightmap
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchain.maxFramesInFlight() * 2);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchain.maxFramesInFlight());

	// Erosion
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[2].descriptorCount = 4; // 2 sets with 2 images

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	// (rendering + erosion computation) = (maxFramesInFlight + 2)
	poolInfo.maxSets = static_cast<uint32_t>(swapchain.maxFramesInFlight() + 2);

	VkResult error_code = vkCreateDescriptorPool(device.handle(), &poolInfo, nullptr, &descriptorPool);
	handleVkResult(error_code, "Failed to allocate descriptor pool");
}

void Renderer::createDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(swapchain.maxFramesInFlight(), descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchain.maxFramesInFlight());
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(swapchain.maxFramesInFlight());
	if (vkAllocateDescriptorSets(device.handle(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < swapchain.maxFramesInFlight(); i++) {
		// --- MVP buffer (binding 0) ---
		VkDescriptorBufferInfo mvpBufferInfo{};
		mvpBufferInfo.buffer = uniformBuffers[i].handle();
		mvpBufferInfo.offset = 0;
		mvpBufferInfo.range = sizeof(MVPBufferObject);

		VkWriteDescriptorSet mvpWrite{};
		mvpWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		mvpWrite.dstSet = descriptorSets[i];
		mvpWrite.dstBinding = 0;
		mvpWrite.dstArrayElement = 0;
		mvpWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		mvpWrite.descriptorCount = 1;
		mvpWrite.pBufferInfo = &mvpBufferInfo;

		// --- MapDetails buffer (binding 1) ---
		VkDescriptorBufferInfo mapDetailsBufferInfo{};
		mapDetailsBufferInfo.buffer = uniformBuffers[i].handle();
		mapDetailsBufferInfo.offset = sizeof(MVPBufferObject); // assuming tightly packed in same buffer
		mapDetailsBufferInfo.range = sizeof(MapDetailsObject);

		VkWriteDescriptorSet mapDetailsWrite{};
		mapDetailsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		mapDetailsWrite.dstSet = descriptorSets[i];
		mapDetailsWrite.dstBinding = 1;
		mapDetailsWrite.dstArrayElement = 0;
		mapDetailsWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		mapDetailsWrite.descriptorCount = 1;
		mapDetailsWrite.pBufferInfo = &mapDetailsBufferInfo;

		// Height sampler (binding 2)
		VkDescriptorImageInfo heightInfo{};
		heightInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		heightInfo.imageView = renderHeightImage.view();
		heightInfo.sampler = renderHeightSampler;

		VkWriteDescriptorSet heightInfoWrite{};
		heightInfoWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		heightInfoWrite.dstSet = descriptorSets[i];
		heightInfoWrite.dstBinding = 2;
		heightInfoWrite.dstArrayElement = 0;
		heightInfoWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		heightInfoWrite.descriptorCount = 1;
		heightInfoWrite.pImageInfo = &heightInfo;

		VkWriteDescriptorSet writes[] = { mvpWrite, mapDetailsWrite, heightInfoWrite };
		vkUpdateDescriptorSets(device.handle(), 3, writes, 0, nullptr);
	}
}


void Renderer::createErosionDescriptorSets() {
	std::array<VkDescriptorSetLayout, 2> layouts = { erosionDescriptorSetLayout, erosionDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 2; // ping-ponging
	allocInfo.pSetLayouts = layouts.data();

	VkResult error_code = vkAllocateDescriptorSets(device.handle(), &allocInfo, erosionDescriptorSets.data());
	handleVkResult(error_code, "Failed to allocate erosion descriptor sets");

	VkDescriptorImageInfo ping1{};
	ping1.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	ping1.imageView = erosionImages[0].view();
	ping1.sampler = VK_NULL_HANDLE;

	VkWriteDescriptorSet ping1Write{};
	ping1Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ping1Write.dstSet = erosionDescriptorSets[0];
	ping1Write.dstBinding = 0;
	ping1Write.dstArrayElement = 0;
	ping1Write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	ping1Write.descriptorCount = 1;
	ping1Write.pImageInfo = &ping1;

	VkDescriptorImageInfo pong1{};
	pong1.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	pong1.imageView = erosionImages[1].view();
	pong1.sampler = VK_NULL_HANDLE;

	VkWriteDescriptorSet pong1Write{};
	pong1Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	pong1Write.dstSet = erosionDescriptorSets[0];
	pong1Write.dstBinding = 1;
	pong1Write.dstArrayElement = 0;
	pong1Write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pong1Write.descriptorCount = 1;
	pong1Write.pImageInfo = &pong1;

	auto ping2 = pong1;
	auto pong2 = ping1;

	VkWriteDescriptorSet ping2Write{};
	ping2Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ping2Write.dstSet = erosionDescriptorSets[1];
	ping2Write.dstBinding = 0;
	ping2Write.dstArrayElement = 0;
	ping2Write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	ping2Write.descriptorCount = 1;
	ping2Write.pImageInfo = &ping2;

	VkWriteDescriptorSet pong2Write{};
	pong2Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	pong2Write.dstSet = erosionDescriptorSets[1];
	pong2Write.dstBinding = 1;
	pong2Write.dstArrayElement = 0;
	pong2Write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pong2Write.descriptorCount = 1;
	pong2Write.pImageInfo = &pong2;

	VkWriteDescriptorSet writes[] = { ping1Write, pong1Write, ping2Write, pong2Write };
	vkUpdateDescriptorSets(device.handle(), 4, writes, 0, nullptr);
}


uint32_t Renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice.handle(), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}


void Renderer::createCommandBuffers() {
	commandBuffers = beginCommands(device.handle(), commandPool, swapchain.maxFramesInFlight());
	erosionCommandBuffers[0] = createSingleCommand(device.handle(), erosionCommandPool);
	erosionCommandBuffers[1] = createSingleCommand(device.handle(), erosionCommandPool);

	DEBUG_LOG << "Successfully created command buffer" << std::endl;
}


void Renderer::recordCommandBuffer(VkCommandBuffer _commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkResult error_code = vkBeginCommandBuffer(_commandBuffer, &beginInfo);
	handleVkResult(error_code, "Failed to begin recording command buffer");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapchain.renderPass();
	renderPassInfo.framebuffer = swapchain.framebuffers()[imageIndex].handle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchain.extent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchain.extent().width;
		viewport.height = (float)swapchain.extent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchain.extent();
		vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { vertexBuffer.handle()};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(_commandBuffer, indexBuffer.handle(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[swapchain.currentFrameIndex()], 0, nullptr);

		vkCmdDrawIndexed(_commandBuffer, static_cast<uint32_t>(indexBuffer.size() / sizeof(float)), 1, 0, 0, 0);
	}
	vkCmdEndRenderPass(_commandBuffer);

	error_code = vkEndCommandBuffer(_commandBuffer);
	handleVkResult(error_code, "Failed to record command buffer");
}


void Renderer::createErosionFence() {
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult error_code = vkCreateFence(device.handle(), &fenceInfo, nullptr, &erosionFence);
	handleVkResult(error_code, "Failed to create erosion fence");
}


void Renderer::beginEroding() {
	setupThread();
}


void Renderer::endEroding() {
	joinThread();
}


void Renderer::runErosionPipeline(uint32_t index) {
	vkWaitForFences(device.handle(), 1, &erosionFence, true, UINT64_MAX);
	vkResetFences(device.handle(), 1, &erosionFence);

	uint32_t nextIndex = (index + 1) % 2;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkResult error_code = vkBeginCommandBuffer(erosionCommandBuffers[index], &beginInfo);
	handleVkResult(error_code, "Failed to begin recording command buffer");

	vkCmdBindPipeline(erosionCommandBuffers[index], VK_PIPELINE_BIND_POINT_COMPUTE, erosionPipeline);

	vkCmdBindDescriptorSets(erosionCommandBuffers[index], VK_PIPELINE_BIND_POINT_COMPUTE, erosionPipelineLayout, 0, 1, &erosionDescriptorSets[index], 0, nullptr);

	vkCmdDispatch(erosionCommandBuffers[index], 64, 64, 1);

	// SET UP FOR SRC COPY
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;       // Compute write must finish
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;     // Copy read can then begin
		barrier.image = erosionImages[nextIndex].handle();
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			erosionCommandBuffers[index],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // Source: Compute stage
			VK_PIPELINE_STAGE_TRANSFER_BIT,       // Destination: Copy stage
			0, 0, nullptr, 0, nullptr, 1, &barrier
		);
	}
	// SET UP FOR DST COPY
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;       
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;     
		barrier.image = renderHeightImage.handle();
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			erosionCommandBuffers[index],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier
		);
	}

	VkImageCopy copyRegion{};
	copyRegion.extent = { renderHeightImage.extent().width, renderHeightImage.extent().height, 1};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };

	// COPY IT
	vkCmdCopyImage(
		erosionCommandBuffers[index],
		erosionImages[nextIndex].handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		renderHeightImage.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyRegion
	);

	// RESTORE FOR ERODING
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;       // Compute write must finish
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;     // Copy read can then begin
		barrier.image = erosionImages[nextIndex].handle();
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			erosionCommandBuffers[index],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // Source: Compute stage
			VK_PIPELINE_STAGE_TRANSFER_BIT,       // Destination: Copy stage
			0, 0, nullptr, 0, nullptr, 1, &barrier
		);
	}
	// RESTORE FOR RENDERING
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;      
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;    
		barrier.image = renderHeightImage.handle();
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			erosionCommandBuffers[index],
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier
		);
	}

	error_code = vkEndCommandBuffer(erosionCommandBuffers[index]);
	handleVkResult(error_code, "Failed to record erosion command buffer");

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &renderCompleteSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &erosionCommandBuffers[index];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &copyCompleteSemaphore;

	device.submitCompute({ submitInfo }, erosionFence);
}


void Renderer::erode() {
	uint32_t index = 0;
	while (erosionRunning) {
		//runErosionPipeline(index);
		index = (index + 1) & 2;
	}
}


void Renderer::setupThread() {
	if (erosionThread.joinable()) return;
	erosionRunning = true;
	erosionThread = std::thread(&Renderer::erode, this);
}


void Renderer::joinThread() {
	if (erosionThread.joinable()) {
		erosionRunning = false;
		erosionThread.join();
		std::cout << "Erosion thread stopped\n";
	}
}


void Renderer::destroy() {
	joinThread();

	vkDestroyFence(device.handle(), erosionFence, nullptr);

	vkDestroySemaphore(device.handle(), copyCompleteSemaphore, nullptr);
	vkDestroySemaphore(device.handle(), renderCompleteSemaphore, nullptr);

	vkDestroyPipeline(device.handle(), graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device.handle(), pipelineLayout, nullptr);

	vkDestroyPipeline(device.handle(), erosionPipeline, nullptr);
	vkDestroyPipelineLayout(device.handle(), erosionPipelineLayout, nullptr);

	swapchain.destroy();

	for (auto& uniformBuffer : uniformBuffers) {
		uniformBuffer.destroy();
	}

	vkDestroyDescriptorPool(device.handle(), descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device.handle(), descriptorSetLayout, nullptr);

	vkDestroyDescriptorSetLayout(device.handle(), erosionDescriptorSetLayout, nullptr);

	vkDestroySampler(device.handle(), renderHeightSampler, nullptr);
	renderHeightImage.destroy();
	erosionImageStager.destroy();
	//gradientImage.destroy();
	for (auto& img : erosionImages) {
		img.destroy();
	}

	indexBuffer.destroy();
	vertexBuffer.destroy();

	vkDestroyCommandPool(device.handle(), commandPool, nullptr);
	vkDestroyCommandPool(device.handle(), erosionCommandPool, nullptr);

	device.destroy();
	surface.destroy();
	instance.destroy();
};

Renderer::~Renderer() {
	this->destroy();
}

void Renderer::updateUniformBuffers(uint32_t currentImage) {
	static auto startTime = std::chrono::steady_clock::now();

	auto currentTime = std::chrono::steady_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	MVPBufferObject mvpData{};
	mvpData.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	mvpData.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	mvpData.proj = glm::perspective(glm::radians(45.0f), swapchain.extent().width / (float)swapchain.extent().height, 0.1f, 10.0f);

	mvpData.proj[1][1] *= -1;

	memcpy(uniformBuffersMapped[currentImage], &mvpData, sizeof(mvpData));

	// write MapDetails right after
	memcpy(
		static_cast<char*>(uniformBuffersMapped[currentImage]) + sizeof(MVPBufferObject),
		&mapDetailsData,
		sizeof(MapDetailsObject)
	);
}

void Renderer::drawFrame() {
	swapchain.waitForFence();
	
	uint32_t imageIndex = swapchain.nextImage();
	vkResetCommandBuffer(commandBuffers[swapchain.currentFrameIndex()], 0);
	recordCommandBuffer(commandBuffers[swapchain.currentFrameIndex()], imageIndex);

	updateUniformBuffers(swapchain.currentFrameIndex());

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[2];
	VkPipelineStageFlags waitStages[2];
	//if (isFirstFrame) {
		waitSemaphores[0] = swapchain.currentImageAvailableSemaphore();
		waitStages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		isFirstFrame = false;
	/*}
	else {
		waitSemaphores[0] = swapchain.currentImageAvailableSemaphore();
		waitSemaphores[1] = copyCompleteSemaphore;
		waitStages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		waitStages[1] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		submitInfo.waitSemaphoreCount = 2;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
	}*/

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[swapchain.currentFrameIndex()];

	/*VkSemaphore signalSemaphores[] = {swapchain.currentRenderFinishedSemaphore(), renderCompleteSemaphore};
	submitInfo.signalSemaphoreCount = 2;*/
	VkSemaphore signalSemaphores[] = { swapchain.currentRenderFinishedSemaphore() };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	device.submitGraphics({ submitInfo }, swapchain.currentFence());

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = { swapchain.handle() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	device.present(&presentInfo);

	swapchain.nextFrame();
}

void Renderer::waitIdle() {
	device.waitIdle();
}

void Renderer::updateTerrain(TerrainGenerator& generator) {
	updateCurrentErosionImage(generator);
}