#include "NekiVK/Core/VulkanGraphicsPipeline.h"
#include <stdexcept>
#include <fstream>

namespace Neki
{



VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VKLogger &_logger, VKDebugAllocator &_deviceDebugAllocator, const VulkanDevice &_device, const VKGraphicsPipelineCleanDesc* _desc, const char* _vertFilepath, const char* _fragFilepath, const char* _tessCtrlFilepath, const char* _tessEvalFilepath, const std::uint32_t _descriptorSetLayoutCount, const VkDescriptorSetLayout* const _descriptorSetLayouts, const std::uint32_t _pushConstantRangeCount, const VkPushConstantRange* const _pushConstantRanges)
													: VulkanPipeline(_logger, _deviceDebugAllocator, _device, _descriptorSetLayoutCount, _descriptorSetLayouts, _pushConstantRangeCount, _pushConstantRanges)
{
	std::vector<const char*> filepaths{ _vertFilepath, _fragFilepath };
	if (_tessCtrlFilepath)
	{
		filepaths.push_back(_tessCtrlFilepath);
		filepaths.push_back(_tessEvalFilepath);
	}
	VulkanGraphicsPipeline::CreateShaderModules(filepaths);
	VulkanGraphicsPipeline::CreatePipeline(_desc);
}



void VulkanGraphicsPipeline::CreateShaderModules(const std::vector<const char*>& _filepaths)
{
	//_filepaths[0] = vertex shader
	//_filepaths[1] = fragment shader
	//
	//optional (and not yet supported):
	//_filepaths[2] = tessellation control shader
	//_filepaths[3] = tessellation evaluation shader
	
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::PIPELINE, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::PIPELINE, "Creating Shader Modules\n");

	if (_filepaths.size() < 2)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, "Vertex and fragment shaders must be specified to create a graphics pipeline\n");
		throw std::runtime_error("");
	}

	if (_filepaths.size() == 3)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, "Both tessellation shaders (control and evaluation) must either be specified or not specified\n");
		throw std::runtime_error("");
	}
	
	if (_filepaths.size() == 4)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, "Tessellation shaders are not currently supported in Vulkaneki\n");
		throw std::runtime_error("");
	}
	
	shaderModules.resize(_filepaths.size());
	

	//Read vertex shader
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Reading vertex shader (" + std::string(_filepaths[0]) + ".spv)", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	std::ifstream vertFile(std::string(_filepaths[0]) + std::string(".spv"), std::ios::ate | std::ios::binary);
	logger.Log(vertFile.is_open() ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, vertFile.is_open() ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
	if (!vertFile.is_open())
	{
		throw std::runtime_error("");
	}
	std::streamsize vertShaderFileSize{ static_cast<std::streamsize>(vertFile.tellg()) };
	std::vector<char> vertShaderBuf(vertShaderFileSize);
	vertFile.seekg(0);
	vertFile.read(vertShaderBuf.data(), vertShaderFileSize);
	vertFile.close();
	
	//Read fragment shader
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Reading fragment shader (" + std::string(_filepaths[1]) + ".spv)", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	std::ifstream fragFile(std::string(_filepaths[1]) + std::string(".spv"), std::ios::ate | std::ios::binary);
	logger.Log(fragFile.is_open() ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, fragFile.is_open() ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
	if (!fragFile.is_open())
	{
		throw std::runtime_error("");
	}
	std::streamsize fragShaderFileSize{ static_cast<std::streamsize>(fragFile.tellg()) };
	std::vector<char> fragShaderBuf(fragShaderFileSize);
	fragFile.seekg(0);
	fragFile.read(fragShaderBuf.data(), fragShaderFileSize);
	fragFile.close();


	//Create vertexShaderModule
	VkShaderModuleCreateInfo vertShaderModuleInfo{};
	vertShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertShaderModuleInfo.pNext = nullptr;
	vertShaderModuleInfo.flags = 0;
	vertShaderModuleInfo.codeSize = vertShaderBuf.size();
	vertShaderModuleInfo.pCode = reinterpret_cast<const std::uint32_t*>(vertShaderBuf.data());
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Creating vertex shader module", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkCreateShaderModule(device.GetDevice(), &vertShaderModuleInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &(shaderModules[0])) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, result == VK_SUCCESS ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, "(" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	//Create fragmentShaderModule
	VkShaderModuleCreateInfo fragShaderModuleInfo{};
	fragShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragShaderModuleInfo.pNext = nullptr;
	fragShaderModuleInfo.flags = 0;
	fragShaderModuleInfo.codeSize = fragShaderBuf.size();
	fragShaderModuleInfo.pCode = reinterpret_cast<const std::uint32_t*>(fragShaderBuf.data());
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Creating fragment shader module", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	result = vkCreateShaderModule(device.GetDevice(), &fragShaderModuleInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &(shaderModules[1]));
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, result == VK_SUCCESS ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, "(" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
}



void VulkanGraphicsPipeline::CreatePipeline(const VKPipelineCleanDesc *_desc)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::PIPELINE, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::PIPELINE, "Creating Graphics Pipeline\n");

	const VKGraphicsPipelineCleanDesc* desc{ dynamic_cast<const VKGraphicsPipelineCleanDesc*>(_desc) };
	
	//Define vertex shader stage
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining vertex shader stage\n");
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = shaderModules[0];
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	//Define fragment shader stage
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining fragment shader stage\n");
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = shaderModules[1];
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	//Todo: add tessellation
	
	VkPipelineShaderStageCreateInfo shaderStages[]{ vertShaderStageInfo, fragShaderStageInfo };

	//Define vertex input
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining vertex input\n");
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.flags = 0;
	vertexInputInfo.vertexBindingDescriptionCount = desc->vertexBindingDescriptionCount;
	vertexInputInfo.pVertexBindingDescriptions = desc->pVertexBindingDescriptions;
	vertexInputInfo.vertexAttributeDescriptionCount = desc->vertexAttributeDescriptionCount;
	vertexInputInfo.pVertexAttributeDescriptions = desc->pVertexAttributeDescriptions;

	//Define input assembly
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining input assembly\n");
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.pNext = nullptr;
	inputAssembly.flags = 0;
	inputAssembly.topology = desc->topology;
	inputAssembly.primitiveRestartEnable = desc->primitiveRestartEnable;
	
	//Define viewport and scissor states
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining viewport and scissor states\n");
	std::vector<VkDynamicState> dynamicStates;
	if (desc->useDynamicViewportState) { dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT); }
	if (desc->useDynamicScissorState)  { dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR); }
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = nullptr;
	dynamicState.flags = 0;
	dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	viewportState.viewportCount = desc->viewportCount;
	viewportState.scissorCount = desc->scissorCount;
	viewportState.pViewports = desc->pViewports;
	viewportState.pScissors = desc->pScissors;

	//Define rasteriser state
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining rasteriser state\n");
	VkPipelineRasterizationStateCreateInfo rasteriser{};
	rasteriser.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasteriser.pNext = nullptr;
	rasteriser.flags = 0;
	rasteriser.depthClampEnable = desc->depthClampEnable;
	rasteriser.rasterizerDiscardEnable = desc->rasteriserDiscardEnable;
	rasteriser.polygonMode = desc->polygonMode;
	rasteriser.lineWidth = desc->lineWidth;
	rasteriser.cullMode = desc->cullMode;
	rasteriser.frontFace = desc->frontFace;
	rasteriser.depthBiasEnable = desc->depthBiasEnable;
	rasteriser.depthBiasConstantFactor = desc->depthBiasConstantFactor;
	rasteriser.depthBiasClamp = desc->depthBiasClamp;
	rasteriser.depthBiasSlopeFactor = desc->depthBiasSlopeFactor;

	//Define multisampling state
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining multisampling state\n");
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.pNext = nullptr;
	multisampling.flags = 0;
	multisampling.sampleShadingEnable = desc->sampleShadingEnable;
	multisampling.rasterizationSamples = desc->rasterisationSamples;
	multisampling.minSampleShading = desc->minSampleShading;
	multisampling.pSampleMask = desc->pSampleMask;
	multisampling.alphaToCoverageEnable = desc->alphaToCoverageEnable;
	multisampling.alphaToOneEnable = desc->alphaToOneEnable;

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining depth-stencil state\n");
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.pNext = nullptr;
	depthStencil.flags = 0;
	depthStencil.depthTestEnable = desc->depthTestEnable;
	depthStencil.depthWriteEnable = desc->depthWriteEnable;
	depthStencil.depthCompareOp = desc->depthCompareOp;
	depthStencil.depthBoundsTestEnable = desc->depthBoundsTestEnable;
	depthStencil.stencilTestEnable = desc->stencilTestEnable;
	depthStencil.front = desc->front;
	depthStencil.back = desc->back;
	depthStencil.minDepthBounds = desc->minDepthBounds;
	depthStencil.maxDepthBounds = desc->maxDepthBounds;
	
	//Define colour blend attachment if desc->pAttachments == nullptr
	VkPipelineColorBlendAttachmentState colourBlendAttachment{};
	if (desc->pAttachments == nullptr)
	{
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining colour blend attachment\n");
		colourBlendAttachment.colorWriteMask = desc->colourWriteMask;
		colourBlendAttachment.blendEnable = desc->blendEnable;
		colourBlendAttachment.srcColorBlendFactor = desc->srcColourBlendFactor;
		colourBlendAttachment.dstColorBlendFactor = desc->dstColourBlendFactor;
		colourBlendAttachment.colorBlendOp = desc->colourBlendOp;
		colourBlendAttachment.srcAlphaBlendFactor = desc->srcAlphaBlendFactor;
		colourBlendAttachment.dstAlphaBlendFactor = desc->dstAlphaBlendFactor;
		colourBlendAttachment.alphaBlendOp = desc->alphaBlendOp;
	}

	//Define colour blend state
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Defining colour blend state\n");
	VkPipelineColorBlendStateCreateInfo colourBlending{};
	colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlending.pNext = nullptr;
	colourBlending.flags = 0;
	colourBlending.logicOpEnable = desc->logicOpEnable;
	colourBlending.logicOp = desc->logicOp;
	colourBlending.attachmentCount = desc->attachmentCount;
	colourBlending.pAttachments = (desc->pAttachments == nullptr ? &colourBlendAttachment : desc->pAttachments);
	colourBlending.blendConstants[0] = desc->blendConstants[0];
	colourBlending.blendConstants[1] = desc->blendConstants[1];
	colourBlending.blendConstants[2] = desc->blendConstants[2];
	colourBlending.blendConstants[3] = desc->blendConstants[3];

	//Create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = 0;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = shaderModules.size();
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasteriser;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colourBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = desc->renderPass;
	pipelineInfo.subpass = desc->subpass;
	pipelineInfo.basePipelineHandle = desc->basePipelineHandle;
	pipelineInfo.basePipelineIndex = desc->basePipelineIndex;
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::PIPELINE, "Creating graphics pipeline", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &pipeline) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, result == VK_SUCCESS ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::PIPELINE, "(" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
}



}