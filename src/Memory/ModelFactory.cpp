#include "NekiVK/Memory/ModelFactory.h"
#include "NekiVK/Memory/ImageFactory.h"
#include "NekiVK/Utils/Strings/format.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Neki
{



ModelFactory::ModelFactory(const VKLogger& _logger, VKDebugAllocator& _deviceDebugAllocator, const VulkanDevice& _device, BufferFactory& _bufferFactory, ImageFactory& _imageFactory, VulkanDescriptorPool& _descriptorPool)
: logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device), bufferFactory(_bufferFactory), imageFactory(_imageFactory), descriptorPool(_descriptorPool)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::MODEL_FACTORY, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::MODEL_FACTORY, "Initialising Model Factory\n");

	constexpr std::size_t numTextureTypes{ static_cast<std::size_t>(MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES) };
	VkDescriptorSetLayoutBinding bindings[numTextureTypes];
	for (std::size_t i{ 0 }; i < numTextureTypes; ++i)
	{
		//Create binding
		bindings[i].binding = i;
		bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[i].descriptorCount = 1;
		bindings[i].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
		bindings[i].pImmutableSamplers = nullptr;
	}

	//Create descriptor set
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.flags = 0;
	layoutInfo.bindingCount = numTextureTypes;
	layoutInfo.pBindings = bindings;
	if (vkCreateDescriptorSetLayout(device.GetDevice(), &layoutInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &materialDescriptorSetLayout) != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::MODEL_FACTORY, "  Failed to create descriptor set layout\n");
		throw std::runtime_error("");
	}

	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::MODEL_FACTORY, "Model Factory Initialised\n");
}



ModelFactory::~ModelFactory()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::MODEL_FACTORY, "Shutting down ModelFactory\n");
}



GPUModel ModelFactory::LoadModel(const char* _filepath, std::unordered_map<MODEL_TEXTURE_TYPE, VkSampler>& _samplers, const VkBufferUsageFlags _vertexBufferFlags, const VkBufferUsageFlags _indexBufferFlags, bool _flipImage)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "Loading 1 Model (" + std::string(_filepath) + ")\n");
	return LoadModelImpl(_filepath, _samplers, _vertexBufferFlags, _indexBufferFlags, _flipImage);
}



std::vector<GPUModel> ModelFactory::LoadModels(std::uint32_t _count, const char** _filepaths, std::unordered_map<MODEL_TEXTURE_TYPE, VkSampler>* _samplers, const VkBufferUsageFlags* _vertexBufferFlags, const VkBufferUsageFlags* _indexBufferFlags, bool* _flipImages)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, "Loading " + std::to_string(_count) + " Model" + std::string(_count == 1 ? "" : "s") + "\n");
	std::vector<GPUModel> gpuModels;
	for (std::size_t i{ 0 }; i < _count; ++i)
	{
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::MODEL_FACTORY, std::string(_filepaths[i]) + "\n");
		gpuModels.push_back(LoadModelImpl(_filepaths[i], _samplers[i], (_vertexBufferFlags == nullptr ? 0 : _vertexBufferFlags[i]), (_indexBufferFlags == nullptr ? 0 : _indexBufferFlags[i]), (_flipImages == nullptr ? false : _flipImages[i])));
	}

	return gpuModels;
}



VkDescriptorSetLayout ModelFactory::GetMaterialDescriptorSetLayout()
{
	return materialDescriptorSetLayout;
}



GPUModel ModelFactory::LoadModelImpl(const char* _filepath, std::unordered_map<MODEL_TEXTURE_TYPE, VkSampler>& _samplers, const VkBufferUsageFlags _vertexBufferFlags, const VkBufferUsageFlags _indexBufferFlags, bool _flipImage)
{
	//Before doing anything, verify _samplers is populated
	for (std::uint32_t i{ 0 }; i < static_cast<std::uint32_t>(MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES); ++i)
	{
		if (!_samplers.contains(static_cast<MODEL_TEXTURE_TYPE>(i)))
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::MODEL_FACTORY, "  _samplers must be fully populated for all texture types - missing " + std::to_string(i) + "\n");
			throw std::runtime_error("");
		}
	}

	GPUModel gpuModel;
	Model cpuModel{ ModelLoader::Load(_filepath) };

	//Load the mesh data
	for (const Mesh& cpuMesh : cpuModel.meshes)
	{
		GPUMesh gpuMesh{};


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
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::MODEL_FACTORY, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
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
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::MODEL_FACTORY, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
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


		//Material index
		gpuMesh.materialIndex = cpuMesh.materialIndex;

		gpuModel.meshes.push_back(gpuMesh);
	}


	//Load the material data
	for (const Material& cpuMaterial : cpuModel.materials)
	{
		//Make a descriptor set where each descriptor corresponds to a texture type
		//The underlying image is an image array of all textures of the texture type
		//The image view format will be identical to that of the image
		GPUMaterial gpuMaterial{};
		const std::size_t numTextureTypes{ cpuMaterial.textures.size() }; //cpuMaterial.textures.size() = number of texture types, not number of total textures - textures[MODEL_TEXTURE_TYPE::DIFFUSE] stores all diffuse textures
		std::vector<VkDescriptorImageInfo> imageInfos(numTextureTypes);
		for (std::size_t i{ 0 }; i < numTextureTypes; ++i)
		{

			//Create image and image view
			TextureInfo texInfo{ cpuMaterial.textures[i] };
			VkImageView imgArrayView{};
			if (texInfo.paths.empty())
			{
				//No textures of this type - use fallback default texture
				ImageMetadata metadata{};
				const char* path{ "Resource Files/DebugTexture.png" };
				VkImage imgArray{ imageFactory.AllocateImageArray(1, &path, VK_IMAGE_USAGE_SAMPLED_BIT, VK_FORMAT_UNDEFINED, MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES, _flipImage, &metadata) };
				imgArrayView = imageFactory.CreateImageView(imgArray, metadata.vkFormat, VK_IMAGE_ASPECT_COLOR_BIT, true, 1);
			}
			else
			{
				//Create an image array (and accompanying view) for all textures of this type
				ImageMetadata metadata{};
				std::vector<const char*> filepathsCStr;
				for (const std::string& s : texInfo.paths) { filepathsCStr.push_back(s.c_str()); }
				VkImage imgArray{ imageFactory.AllocateImageArray(texInfo.paths.size(), filepathsCStr.data(), VK_IMAGE_USAGE_SAMPLED_BIT, VK_FORMAT_UNDEFINED, texInfo.type, _flipImage, &metadata) };
				imgArrayView = imageFactory.CreateImageView(imgArray, metadata.vkFormat, VK_IMAGE_ASPECT_COLOR_BIT, true, texInfo.paths.size());
			}

			//Create image info
			imageInfos[i].imageView = imgArrayView;
			imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfos[i].sampler = _samplers[texInfo.type];
		}

		//Create descriptor set
		gpuMaterial.descriptorSet = descriptorPool.AllocateDescriptorSet(materialDescriptorSetLayout);

		//Bind descriptors
		std::vector<VkWriteDescriptorSet> descriptorWrites(numTextureTypes);
		for (std::size_t i{ 0 }; i < numTextureTypes; ++i)
		{
			descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[i].pNext = nullptr;
			descriptorWrites[i].dstSet = gpuMaterial.descriptorSet;
			descriptorWrites[i].dstBinding = i;
			descriptorWrites[i].dstArrayElement = 0;
			descriptorWrites[i].descriptorCount = 1;
			descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[i].pImageInfo = &imageInfos[i];
			descriptorWrites[i].pTexelBufferView = nullptr;
			descriptorWrites[i].pBufferInfo = nullptr;
		}
		vkUpdateDescriptorSets(device.GetDevice(), numTextureTypes, descriptorWrites.data(), 0, nullptr);

		gpuModel.materials.push_back(gpuMaterial);
	}

	return gpuModel;
}


}