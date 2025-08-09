#ifndef VULKANGRAPHICSPIPELINE_H
#define VULKANGRAPHICSPIPELINE_H

#include "VulkanPipeline.h"

namespace Neki
{


struct VKGraphicsPipelineCleanDesc final : VKPipelineCleanDesc
{
	//Shader stages
	bool useTessellation{ false };

	//Vertex attributes
	std::uint32_t vertexBindingDescriptionCount{ 0 };
	VkVertexInputBindingDescription* pVertexBindingDescriptions{ nullptr };
	std::uint32_t vertexAttributeDescriptionCount{ 0 };
	VkVertexInputAttributeDescription* pVertexAttributeDescriptions{ nullptr };

	//Input assembly
	VkPrimitiveTopology topology{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
	VkBool32 primitiveRestartEnable{ VK_FALSE };

	//Tessellation (not currently supported)
	std::uint32_t patchControlPoints;

	//Viewport and scissor
	bool useDynamicViewportState{ true };
	bool useDynamicScissorState{ true };
	std::uint32_t viewportCount{ 1 };
	std::uint32_t scissorCount{ 1 };
	VkViewport* pViewports{ nullptr };
	VkRect2D* pScissors{ nullptr };

	//Rasteriser
	VkBool32 depthClampEnable{ VK_FALSE };
	VkBool32 rasteriserDiscardEnable{ VK_FALSE };
	VkPolygonMode polygonMode{ VK_POLYGON_MODE_FILL };
	float lineWidth{ 1.0f };
	VkCullModeFlags cullMode{ VK_CULL_MODE_BACK_BIT };
	VkFrontFace frontFace{ VK_FRONT_FACE_CLOCKWISE };
	VkBool32 depthBiasEnable{ VK_FALSE };
	float depthBiasConstantFactor{ 0.0f };
	float depthBiasClamp{ 0.0f };
	float depthBiasSlopeFactor{ 0.0f };

	//Multisampling
	VkBool32 sampleShadingEnable{ VK_FALSE };
	VkSampleCountFlagBits rasterisationSamples{ VK_SAMPLE_COUNT_1_BIT };
	float minSampleShading{ 1.0f };
	VkSampleMask* pSampleMask{ nullptr };
	VkBool32 alphaToCoverageEnable{ VK_FALSE };
	VkBool32 alphaToOneEnable{ VK_FALSE };

	//Depth and stencil state
	VkBool32 depthTestEnable{ VK_TRUE };
	VkBool32 depthWriteEnable{ VK_TRUE };
	VkCompareOp depthCompareOp{ VK_COMPARE_OP_LESS };
	VkBool32 depthBoundsTestEnable{ VK_FALSE };
	VkBool32 stencilTestEnable{ VK_FALSE };
	VkStencilOpState front{};
	VkStencilOpState back{};
	float minDepthBounds{ 0.0f };
	float maxDepthBounds{ 1.0f };
	
	//Colour blend attachment (leave empty if providing your own)
	VkColorComponentFlags colourWriteMask{ VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };
	VkBool32 blendEnable{ VK_FALSE };
	VkBlendFactor srcColourBlendFactor{ VK_BLEND_FACTOR_ONE };
	VkBlendFactor dstColourBlendFactor{ VK_BLEND_FACTOR_ZERO };
	VkBlendOp colourBlendOp{ VK_BLEND_OP_ADD };
	VkBlendFactor srcAlphaBlendFactor{ VK_BLEND_FACTOR_ONE };
	VkBlendFactor dstAlphaBlendFactor{ VK_BLEND_FACTOR_ZERO };
	VkBlendOp alphaBlendOp{ VK_BLEND_OP_ADD };

	//Colour blend state (if pAttachments is left as nullptr, the default attachment seen directly above will be used)
	VkBool32 logicOpEnable{ VK_FALSE };
	VkLogicOp logicOp{ VK_LOGIC_OP_COPY };
	std::uint32_t attachmentCount{ 1 };
	VkPipelineColorBlendAttachmentState* pAttachments{ nullptr };
	float blendConstants[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

	//Passes - renderPass has no default value and must be filled
	VkRenderPass renderPass;
	std::uint32_t subpass{ 0 };

	//Base pipeline
	VkPipeline basePipelineHandle{ VK_NULL_HANDLE };
	std::int32_t basePipelineIndex{ -1 };
};


class VulkanGraphicsPipeline final : public VulkanPipeline
{
public:
	//Filepaths are relative to output directory (e.g.: for {OUTPUT_FOLDER}/SubFolder/vert.glsl, pass in "SubFolder/vert.glsl")
	VulkanGraphicsPipeline(const VKLogger& _logger,
	                       VKDebugAllocator& _deviceDebugAllocator,
	                       const VulkanDevice& _device,
	                       const VKGraphicsPipelineCleanDesc* _desc,
	                       const char* _vertFilepath,
	                       const char* _fragFilepath,
	                       const char* _tessCtrlFilepath=nullptr,
	                       const char* _tessEvalFilepath=nullptr,
	                       const std::uint32_t _descriptorSetLayoutCount=0,
	                       const VkDescriptorSetLayout* const _descriptorSetLayouts=nullptr,
	                       const std::uint32_t _pushConstantRangeCount=0,
	                       const VkPushConstantRange* const _pushConstantRanges=nullptr);

	~VulkanGraphicsPipeline() override = default;

private:
	void CreateShaderModules(const std::vector<const char*>& _filepaths) override;
	void CreatePipeline(const VKPipelineCleanDesc *_desc) override;
};



}

#endif