#ifndef MODELFACTORY_H
#define MODELFACTORY_H



#include "BufferFactory.h"
#include "../Utils/Loaders/ModelLoader.h"
#include "NekiVK/Core/VulkanDescriptorPool.h"


//Helper class to load models into a vector of GPUMesh objects
namespace Neki
{


class ImageFactory;


struct GPUMaterial
{
	//Contains all image views for a material
	VkDescriptorSet descriptorSet;
};


struct GPUMesh
{
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;
	std::uint32_t indexCount;
	std::size_t materialIndex; //Index into parent GPUModel's materials vector
};

struct GPUModel
{
	std::vector<GPUMesh> meshes;
	std::vector<GPUMaterial> materials;
};


class ModelFactory
{
public:
	explicit ModelFactory(const VKLogger& _logger,
	                      VKDebugAllocator& _deviceDebugAllocator,
	                      const VulkanDevice& _device,
	                      BufferFactory& _bufferFactory,
	                      ImageFactory& _imageFactory,
	                      VulkanDescriptorPool& _descriptorPool);

	~ModelFactory();


	//Load data for a single model at _filepath into a vector of GPUMeshes comprising the model
	//Samplers for all texture types must be set in _samplers
	//Optionally pass in additional flags for the vertex and index buffers (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT and VK_BUFFER_USAGE_INDEX_BUFFER_BIT are added automatically)
	[[nodiscard]] GPUModel LoadModel(const char* _filepath, std::unordered_map<MODEL_TEXTURE_TYPE, VkSampler>& _samplers, const VkBufferUsageFlags _vertexBufferFlags = 0, const VkBufferUsageFlags _indexBufferFlags = 0, bool _flipImage = false);

	//Load model data for _count models at _filepaths into a vector of vectors of GPUMeshes comprising the corresponding model
	//E.g.: LoadModel(...)[1][2] is mesh index 2 of model index 1
	//Samplers for all texture types for all models must be set in _samplers
	//Optionally pass in additional flags for the vertex and index buffers (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT and VK_BUFFER_USAGE_INDEX_BUFFER_BIT are added automatically)
	[[nodiscard]] std::vector<GPUModel> LoadModels(std::uint32_t _count, const char** _filepaths, std::unordered_map<MODEL_TEXTURE_TYPE, VkSampler>* _samplers, const VkBufferUsageFlags* _vertexBufferFlags = nullptr, const VkBufferUsageFlags* _indexBufferFlags = nullptr, bool* _flipImages = nullptr);


	[[nodiscard]] VkDescriptorSetLayout GetMaterialDescriptorSetLayout();


private:
	[[nodiscard]] GPUModel LoadModelImpl(const char* _filepath, std::unordered_map<MODEL_TEXTURE_TYPE, VkSampler>& _samplers, const VkBufferUsageFlags _vertexBufferFlags, const VkBufferUsageFlags _indexBufferFlags, bool _flipImage);

	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;
	BufferFactory& bufferFactory;
	ImageFactory& imageFactory;
	VulkanDescriptorPool& descriptorPool;

	VkDescriptorSetLayout materialDescriptorSetLayout{};
};



}



#endif