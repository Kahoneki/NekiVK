#include "NekiVK/Memory/ModelFactory.h"
#include "../../include/NekiVK/Utils/Strings/format.h"

#include <stdexcept>
#include <cstring>

namespace Neki
{



ModelFactory::ModelFactory(const VKLogger& _logger, VKDebugAllocator& _deviceDebugAllocator, const VulkanDevice& _device, BufferFactory& _bufferFactory)
						  : logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device), bufferFactory(_bufferFactory)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::MODEL_FACTORY, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::MODEL_FACTORY, "Model Factory Initialised\n");
}



ModelFactory::~ModelFactory()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::MODEL_FACTORY,"Shutting down ModelFactory\n");
}



std::vector<GPUMesh> ModelFactory::LoadModel(const char* _filepath, const VkBufferUsageFlags _vertexBufferFlags, const VkBufferUsageFlags _indexBufferFlags)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY,"Loading 1 Model (" + std::string(_filepath) + ")\n");
	return LoadModelImpl(_filepath, _vertexBufferFlags, _indexBufferFlags);
}



std::vector<std::vector<GPUMesh>> ModelFactory::LoadModels(std::uint32_t _count, const char** _filepaths, const VkBufferUsageFlags* _vertexBufferFlags, const VkBufferUsageFlags* _indexBufferFlags)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY,"Loading " + std::to_string(_count) + " Model" + std::string(_count == 1 ? "" : "s") +"\n");
	std::vector<std::vector<GPUMesh>> gpuModels;
	for (std::size_t i{ 0 }; i<_count; ++i)
	{
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, std::string(_filepaths[i]) + "\n");
		gpuModels.push_back(LoadModelImpl(_filepaths[i], (_vertexBufferFlags == nullptr ? 0 : _vertexBufferFlags[i]), (_indexBufferFlags == nullptr ? 0 : _indexBufferFlags[i])));
	}
	
	return gpuModels;
}



std::vector<GPUMesh> ModelFactory::LoadModelImpl(const char* _filepath, const VkBufferUsageFlags _vertexBufferFlags, const VkBufferUsageFlags _indexBufferFlags)
{
	std::vector<GPUMesh> gpuMeshes;

	//Load the model
	Model model{ ModelLoader::Load(_filepath) };
	for (const Mesh& cpuMesh : model.meshes)
	{
		//Create a GPUMesh for the current CPU mesh
		GPUMesh gpuMesh;

		//Create the vertex buffer
		VkDeviceSize vertexBufferSize{ sizeof(ModelVertex) * cpuMesh.vertices.size() };
		gpuMesh.vertexBuffer = bufferFactory.AllocateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::MODEL_FACTORY, "  Vertex buffer allocated (" + std::to_string(cpuMesh.vertices.size()) + " vertices - " + GetFormattedSizeString(sizeof(ModelVertex) * cpuMesh.vertices.size()) + ")\n");
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "  Populating vertex buffer\n");
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "    Mapping memory", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
		void* vertexBufferMap;
		VkMemoryRequirements vertexBufferMemoryRequirements;
		vkGetBufferMemoryRequirements(device.GetDevice(), gpuMesh.vertexBuffer, &vertexBufferMemoryRequirements);
		VkResult result{ vkMapMemory(device.GetDevice(), bufferFactory.GetMemory(gpuMesh.vertexBuffer), 0, vertexBufferMemoryRequirements.size, 0, &vertexBufferMap) };
		logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::MODEL_FACTORY, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
		if (result != VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::MODEL_FACTORY," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
			throw std::runtime_error("");
		}
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "    Writing to map\n");
		memcpy(vertexBufferMap, cpuMesh.vertices.data(), static_cast<std::size_t>(vertexBufferSize));
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::MODEL_FACTORY, "    Buffer memory filled with vertex data\n");
		vkUnmapMemory(device.GetDevice(), bufferFactory.GetMemory(gpuMesh.vertexBuffer));
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::MODEL_FACTORY, "    Buffer memory unmapped\n");
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "  Transferring host-side vertex temp-staging-buffer to device-local memory\n");
		gpuMesh.vertexBuffer = bufferFactory.TransferToDeviceLocalBuffer(gpuMesh.vertexBuffer, true);

		//Create the index buffer
		VkDeviceSize indexBufferSize{ sizeof(std::uint32_t) * cpuMesh.indices.size() };
		gpuMesh.indexBuffer = bufferFactory.AllocateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::MODEL_FACTORY, "  Index buffer allocated (" + std::to_string(cpuMesh.indices.size()) + " indices - " + GetFormattedSizeString(sizeof(std::uint32_t) * cpuMesh.indices.size()) + ")\n");
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "  Populating index buffer\n");
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "    Mapping memory", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
		void* indexBufferMap;
		VkMemoryRequirements indexBufferMemoryRequirements;
		vkGetBufferMemoryRequirements(device.GetDevice(), gpuMesh.indexBuffer, &indexBufferMemoryRequirements);
		result = vkMapMemory(device.GetDevice(), bufferFactory.GetMemory(gpuMesh.indexBuffer), 0, indexBufferMemoryRequirements.size, 0, &indexBufferMap);
		logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::MODEL_FACTORY, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
		if (result != VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::MODEL_FACTORY," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
			throw std::runtime_error("");
		}
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "    Writing to map\n");
		memcpy(indexBufferMap, cpuMesh.indices.data(), static_cast<std::size_t>(indexBufferSize));
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::MODEL_FACTORY, "    Buffer memory filled with vertex data\n");
		vkUnmapMemory(device.GetDevice(), bufferFactory.GetMemory(gpuMesh.indexBuffer));
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::MODEL_FACTORY, "    Buffer memory unmapped\n");
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "  Transferring host-side vertex temp-staging-buffer to device-local memory\n");
		gpuMesh.indexBuffer = bufferFactory.TransferToDeviceLocalBuffer(gpuMesh.indexBuffer, true);
		gpuMesh.indexCount = cpuMesh.indices.size();

		gpuMeshes.push_back(gpuMesh);
	}
	
	return gpuMeshes;
}



}