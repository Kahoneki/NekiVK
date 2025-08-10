#ifndef MODELTEST_H
#define MODELTEST_H


#include <NekiVK/NekiVK.h>
#include <memory>


class ModelTest
{
public:
	ModelTest();
	~ModelTest() = default;

	void Run();
	
	
private:
	void CreateRenderManager();
	void LoadModel();
	void InitialiseCamData();
	void CreateDescriptorSet();
	void BindDescriptorSet() const;
	void CreatePipeline();

	
	std::unique_ptr<Neki::VKLogger> logger;
	std::unique_ptr<Neki::VKDebugAllocator> instDebugAllocator;
	std::unique_ptr<Neki::VKDebugAllocator> deviceDebugAllocator;
	std::unique_ptr<Neki::VulkanDevice> vulkanDevice;
	std::unique_ptr<Neki::VulkanCommandPool> vulkanCommandPool;
	std::unique_ptr<Neki::VulkanDescriptorPool> vulkanDescriptorPool;
	std::unique_ptr<Neki::BufferFactory> bufferFactory;
	std::unique_ptr<Neki::ImageFactory> imageFactory;
	std::unique_ptr<Neki::ModelFactory> modelFactory;
	std::unique_ptr<Neki::VulkanSwapchain> vulkanSwapchain;
	std::unique_ptr<Neki::VulkanRenderManager> vulkanRenderManager;
	std::unique_ptr<Neki::VulkanGraphicsPipeline> vulkanGraphicsPipeline;

	VkBuffer camDataUBO;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	Neki::GPUMesh cubeMesh;
	glm::mat4 cubeModelMatrix;
};

#endif