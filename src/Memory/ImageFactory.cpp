#include "NekiVK/Memory/ImageFactory.h"
#include "NekiVK/Utils/Strings/format.h"

#include <cstring>
#include <climits>
#include <stdexcept>
#include <algorithm>
#include <stb_image.h>


namespace Neki
{



ImageFactory::ImageFactory(const VKLogger& _logger, VKDebugAllocator& _deviceDebugAllocator, const VulkanDevice& _device, VulkanCommandPool& _commandPool, BufferFactory& _bufferFactory)
: logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device), commandPool(_commandPool), bufferFactory(_bufferFactory)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::IMAGE_FACTORY, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::IMAGE_FACTORY, "Image Factory Initialised\n");
}



ImageFactory::~ImageFactory()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::IMAGE_FACTORY, "Shutting down ImageFactory\n");
	while (!imageViewImageMap.empty())
	{
		VkImageView imgView{ imageViewImageMap.begin()->first };
		FreeImageViewImpl(imgView);
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::IMAGE_FACTORY, "  All image views freed\n");
	while (!imageMemoryMap.empty())
	{
		VkImage img{ imageMemoryMap.begin()->first };
		FreeImageImpl(img);
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::IMAGE_FACTORY, "  All images and underlying memory freed\n");
	while (!samplers.empty())
	{
		//Get sampler from back for efficient removal from vector
		VkSampler sampler = samplers.back();
		FreeSamplerImpl(sampler);
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::IMAGE_FACTORY, "  All images and underlying memory freed\n");
}



VkImage ImageFactory::AllocateImage(const char* _filepath, const VkImageUsageFlags _flags, VkFormat _formatOverride, MODEL_TEXTURE_TYPE _textureType, bool _flipImage, ImageMetadata* _out_metadata)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Allocating 1 Image And Associated Memory\n");
	return AllocateImageImpl(_filepath, _flags, _formatOverride, _textureType, _flipImage, _out_metadata);
}



VkImage ImageFactory::AllocateImage(VkExtent2D _size, VkFormat _format, const VkImageUsageFlags _flags)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Allocating 1 Empty Image And Associated Memory\n");
	return AllocateImageImpl(_size, _format, _flags);
}



std::vector<VkImage> ImageFactory::AllocateImages(std::uint32_t _count, const char** _filepaths, const VkImageUsageFlags* _flags, VkFormat* _formatOverrides, MODEL_TEXTURE_TYPE* _textureTypes, bool* _flipImages, ImageMetadata* _out_metadata)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Allocating " + std::to_string(_count) + " Image" + std::string(_count == 1 ? "" : "s") + " And Associated Memory\n", VK_LOGGER_WIDTH::DEFAULT, false);
	std::vector<VkImage> images;
	for (std::size_t i{ 0 }; i < _count; ++i)
	{
		VkFormat formatOverride{ _formatOverrides == nullptr ? VK_FORMAT_UNDEFINED : _formatOverrides[i] };
		MODEL_TEXTURE_TYPE textureType{ _textureTypes == nullptr ? MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES : _textureTypes[i] };
		bool flipImage{ _flipImages == nullptr ? false : _flipImages[i] };
		images.push_back(AllocateImageImpl(_filepaths[i], _flags[i], formatOverride, textureType, flipImage, &(_out_metadata[i])));
	}
	return images;
}



std::vector<VkImage> ImageFactory::AllocateImages(std::uint32_t _count, const VkExtent2D* _sizes, const VkFormat* _formats, const VkImageUsageFlags* _flags, bool* _flipImages)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Allocating " + std::to_string(_count) + " Empty Image" + std::string(_count == 1 ? "" : "s") + " And Associated Memory\n", VK_LOGGER_WIDTH::DEFAULT, false);
	std::vector<VkImage> images;
	for (std::size_t i{ 0 }; i < _count; ++i)
	{
		images.push_back(AllocateImageImpl(_sizes[i], _formats[i], _flags[i], _flipImages == nullptr ? false : _flipImages[i]));
	}
	return images;
}



VkImage ImageFactory::AllocateImageArray(std::uint32_t _arrSize, const char** _filepaths, const VkImageUsageFlags _flags, VkFormat _formatOverride, MODEL_TEXTURE_TYPE _textureType, bool _flipImage, ImageMetadata* _out_metadata)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Allocating Image Array Of Size " + std::to_string(_arrSize) + " And Associated Memory\n", VK_LOGGER_WIDTH::DEFAULT, false);
	return AllocateImageArrayImpl(_arrSize, _filepaths, _flags, _formatOverride, _textureType, _flipImage, _out_metadata);
}



void ImageFactory::FreeImage(VkImage& _image)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Freeing 1 Image And Associated Memory\n");
	FreeImageImpl(_image);
}



void ImageFactory::FreeImages(std::uint32_t _count, VkImage* _images)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Freeing " + std::to_string(_count) + " Image" + std::string(_count == 1 ? "" : "s") + " And Associated Memory\n", VK_LOGGER_WIDTH::DEFAULT, false);
	for (std::size_t i{ 0 }; i < _count; ++i)
	{
		FreeImageImpl(_images[i]);
	}
}



void ImageFactory::TransitionImage(VkImageLayout _srcLayout, VkImageLayout _dstLayout, VkImageAspectFlags _aspectMask, VkAccessFlags _srcAccessMask, VkAccessFlags _dstAccessMask, VkPipelineStageFlags _srcStageMask, VkPipelineStageFlags _dstStageMask, std::size_t _layers, VkImage& _image, VkCommandBuffer* _commandBuffer)
{
	VkCommandBuffer commandBuffer{ _commandBuffer == nullptr ? commandPool.AllocateCommandBuffer() : *_commandBuffer };
	if (_commandBuffer == nullptr)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
	}

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.oldLayout = _srcLayout;
	barrier.newLayout = _dstLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = _image;
	barrier.subresourceRange.aspectMask = _aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = _layers;
	barrier.srcAccessMask = _srcAccessMask;
	barrier.dstAccessMask = _dstAccessMask;
	vkCmdPipelineBarrier(commandBuffer, _srcStageMask, _dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	if (_commandBuffer == nullptr)
	{
		vkEndCommandBuffer(commandBuffer);

		//Execute the command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device.GetGraphicsQueue());
		commandPool.FreeCommandBuffer(commandBuffer);
	}
}



VkImageView ImageFactory::CreateImageView(const VkImage& _image, const VkFormat& _format, const VkImageAspectFlags& _aspectFlags, bool _arrayView, std::uint32_t _layerCount)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Creating 1 Image View\n");
	return CreateImageViewImpl(_image, _format, _aspectFlags, _arrayView, _layerCount);
}



std::vector<VkImageView> ImageFactory::CreateImageViews(std::uint32_t _count, const VkImage* _images, const VkFormat* _formats, const VkImageAspectFlags* _aspectFlags, bool* _arrayViews, std::uint32_t* _layerCounts)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Creating " + std::to_string(_count) + " Image View" + std::string(_count == 1 ? "" : "s") + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	std::vector<VkImageView> imageViews;
	for (std::size_t i{ 0 }; i < _count; ++i)
	{
		VkImageAspectFlags flags{ _aspectFlags == nullptr ? 0 : _aspectFlags[i] };
		bool arrayView{ _arrayViews == nullptr ? false : _arrayViews[i] };
		std::uint32_t layerCount{ _layerCounts == nullptr ? 1 : _layerCounts[i] };
		imageViews.push_back(CreateImageViewImpl(_images[i], _formats[i], flags, arrayView, layerCount));
	}
	return imageViews;
}



void ImageFactory::FreeImageView(VkImageView& _imageView)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Freeing 1 Image View\n", VK_LOGGER_WIDTH::DEFAULT, false);
	FreeImageViewImpl(_imageView);
}



void ImageFactory::FreeImageViews(std::uint32_t _count, VkImageView* _imageViews)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Freeing " + std::to_string(_count) + " Image View" + std::string(_count == 1 ? "" : "s") + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	for (std::size_t i{ 0 }; i < _count; ++i)
	{
		FreeImageViewImpl(_imageViews[i]);
	}
}



VkSampler ImageFactory::CreateSampler(const VkSamplerCreateInfo& _createInfo)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Creating 1 Sampler\n");
	return CreateSamplerImpl(_createInfo);
}



std::vector<VkSampler> ImageFactory::CreateSamplers(std::uint32_t _count, const VkSamplerCreateInfo* _createInfos)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Creating " + std::to_string(_count) + " Sampler" + std::string(_count == 1 ? "" : "s") + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	std::vector<VkSampler> samplers;
	for (std::size_t i{ 0 }; i < _count; ++i)
	{
		samplers.push_back(CreateSamplerImpl(_createInfos[i]));
	}
	return samplers;
}



void ImageFactory::FreeSampler(VkSampler& _sampler)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Freeing 1 Sampler\n", VK_LOGGER_WIDTH::DEFAULT, false);
	FreeSamplerImpl(_sampler);
}



void ImageFactory::FreeSampler(std::uint32_t _count, VkSampler* _samplers)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "Freeing " + std::to_string(_count) + " Sampler" + std::string(_count == 1 ? "" : "s") + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	for (std::size_t i{ 0 }; i < _count; ++i)
	{
		FreeSamplerImpl(_samplers[i]);
	}
}



VkFormat ImageFactory::ChooseFormat(int _nrChannels, VkFormat _formatOverride, MODEL_TEXTURE_TYPE _textureType)
{
	//If format override is specified, prioritise it over anything else
	if (_formatOverride != VK_FORMAT_UNDEFINED)
	{
		return _formatOverride;
	}

	//For colour maps (or if texture type hasn't been set), always use SRGB
	if (_textureType == MODEL_TEXTURE_TYPE::DIFFUSE || _textureType == MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES)
	{
		switch (_nrChannels)
		{
		case 1: return VK_FORMAT_R8_SRGB;
		case 2: return VK_FORMAT_R8G8_SRGB;
		case 3: return VK_FORMAT_R8G8B8_SRGB;
		case 4: return VK_FORMAT_R8G8B8A8_SRGB;
		default: return VK_FORMAT_R8G8B8A8_SRGB; //Fallback
		}
	}

	//For all other (data) maps (normals, specular, roughness, etc.), always use UNORM
	switch (_nrChannels)
	{
	case 1: return VK_FORMAT_R8_UNORM;
	case 2: return VK_FORMAT_R8G8_UNORM;
	case 3: return VK_FORMAT_R8G8B8_UNORM;
	case 4: return VK_FORMAT_R8G8B8A8_UNORM;
	default: return VK_FORMAT_R8G8B8A8_UNORM; //Fallback
	}
}



VkImage ImageFactory::AllocateImageImpl(const char* _filepath, const VkImageUsageFlags _flags, VkFormat _formatOverride, MODEL_TEXTURE_TYPE _textureType, bool _flipImage, ImageMetadata* _out_metadata)
{
	//Load the data to disk
	ImageData imgData{ ImageLoader::Load(_filepath, _flipImage) };
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Loaded " + std::string(_filepath) + " from disk (" + std::to_string(imgData.metadata.width) + "x" + std::to_string(imgData.metadata.height) + ", " + std::to_string(imgData.metadata.channels) + " channels)\n");

	//Create temporary staging buffer
	const VkDeviceSize imgSize{ static_cast<VkDeviceSize>(imgData.metadata.width * imgData.metadata.height * imgData.metadata.channels) }; //Assume 1 byte per channel
	VkBuffer stagingBuffer{ bufferFactory.AllocateBuffer(imgSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) };
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Created temporary staging buffer of size " + GetFormattedSizeString(imgSize) + "\n");

	//Copy pixel data to staging buffer
	void* mappedMemory;
	vkMapMemory(device.GetDevice(), bufferFactory.GetMemory(stagingBuffer), 0, imgSize, 0, &mappedMemory);
	memcpy(mappedMemory, imgData.pixels, imgSize);
	vkUnmapMemory(device.GetDevice(), bufferFactory.GetMemory(stagingBuffer));
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Pixel Data copied to staging buffer\n");

	//Free the image data as it's in the staging buffer now
	ImageLoader::Free(imgData.pixels);

	//Create destination image in device local memory
	VkFormat format{ ChooseFormat(imgData.metadata.channels, _formatOverride, _textureType) };
	imgData.metadata.vkFormat = format;
	VkImage image{ AllocateImageImpl(VkExtent2D(imgData.metadata.width, imgData.metadata.height), format, _flags) };

	//Record command buffer for copy command
	VkCommandBuffer commandBuffer{ commandPool.AllocateCommandBuffer() };
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	TransitionImage(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 1, image, &commandBuffer);

	//Copy the staging buffer to the image
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { static_cast<std::uint32_t>(imgData.metadata.width), static_cast<std::uint32_t>(imgData.metadata.height), 1 };
	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 1, image, &commandBuffer);

	vkEndCommandBuffer(commandBuffer);

	//Execute the command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(device.GetGraphicsQueue());


	//Cleanup
	commandPool.FreeCommandBuffer(commandBuffer);
	bufferFactory.FreeBuffer(stagingBuffer);

	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Texture data successfully transferred to device local memory\n");

	if (_out_metadata != nullptr) { *_out_metadata = imgData.metadata; }

	return image;
}



VkImage ImageFactory::AllocateImageImpl(VkExtent2D _size, VkFormat _format, const VkImageUsageFlags _flags, std::size_t _layers)
{
	VkImageCreateInfo imgInfo{};
	imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgInfo.pNext = nullptr;
	imgInfo.flags = 0;
	imgInfo.imageType = VK_IMAGE_TYPE_2D;
	imgInfo.extent.width = _size.width;
	imgInfo.extent.height = _size.height;
	imgInfo.extent.depth = 1;
	imgInfo.mipLevels = 1;
	imgInfo.arrayLayers = _layers;
	imgInfo.format = _format;
	imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | _flags;
	imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Creating image", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkImage image;
	VkResult result{ vkCreateImage(device.GetDevice(), &imgInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &image) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, "(" + std::to_string(result) + ")", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	//Find a memory type that is DEVICE_LOCAL
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Searching for compatible memory type for image that is DEVICE_LOCAL", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device.GetDevice(), image, &memRequirements);
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(device.GetPhysicalDevice(), &memProperties);
	std::uint32_t memTypeIndex{ UINT_MAX };
	for (std::uint32_t i{ 0 }; i < memProperties.memoryTypeCount; ++i)
	{
		if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			memTypeIndex = i;
			break;
		}
	}
	logger.Log(memTypeIndex != UINT_MAX ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, memTypeIndex != UINT_MAX ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (memTypeIndex == UINT_MAX)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, "(" + std::to_string(result) + ")", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memTypeIndex;
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Allocating DEVICE_LOCAL memory for image", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	result = vkAllocateMemory(device.GetDevice(), &allocInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &(imageMemoryMap[image]));
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, "(" + std::to_string(result) + ")", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	//Bind the allocated memory to the VkImage handle
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Binding allocated memory to image\n");
	vkBindImageMemory(device.GetDevice(), image, imageMemoryMap[image], 0);

	return image;
}



VkImage ImageFactory::AllocateImageArrayImpl(std::uint32_t _arrSize, const char** _filepaths, const VkImageUsageFlags _flags, VkFormat _formatOverride, MODEL_TEXTURE_TYPE _textureType, bool _flipImage, ImageMetadata* _out_metadata)
{
	//Images in an image array must all have the same format and dimensions
	//Require format to be the same across all images
	//Pad dimensions of all images to be max dimension sizes

	//Get max dimensions while loading all image data into a vector (I would use stbi_info for this but it's broken for pngs - returns "no SOI" stbi_failure_reason())...
	//...They said they fixed this in 2022....
	std::vector<ImageData> imageData(_arrSize);
	imageData[0] = ImageLoader::Load(_filepaths[0], _flipImage);
	int maxWidth{ imageData[0].metadata.width };
	int maxHeight{ imageData[0].metadata.height };
	int requiredNrChannels{ imageData[0].metadata.channels };
	VkFormat format{ ChooseFormat(requiredNrChannels, _formatOverride, _textureType) };
	imageData[0].metadata.vkFormat = format;
	for (std::size_t i{ 1 }; i < _arrSize; ++i)
	{
		imageData[i] = ImageLoader::Load(_filepaths[i], _flipImage);
		int width{ imageData[i].metadata.width };
		int height{ imageData[i].metadata.height };
		int nrChannels{ imageData[i].metadata.channels };
		imageData[i].metadata.vkFormat = format;
		if (nrChannels != requiredNrChannels && _formatOverride == VK_FORMAT_UNDEFINED)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, "Number of channels for image " + std::to_string(i) + " (" + std::to_string(nrChannels) + ") does not match required number of channels (" + std::to_string(requiredNrChannels) + ")\n");
			throw std::runtime_error("");
		}

		if (width > maxWidth) { maxWidth = width; }
		if (height > maxHeight) { maxHeight = height; }
	}


	//Create image array
	VkImage imageArray{ AllocateImageImpl(VkExtent2D(maxWidth, maxHeight), format, _flags, _arrSize) };

	//Create temporary staging buffer
	const VkDeviceSize imgSize{ static_cast<VkDeviceSize>(maxWidth * maxHeight * requiredNrChannels) }; //Assume 1 byte per channel
	VkBuffer stagingBuffer{ bufferFactory.AllocateBuffer(imgSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) };
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Created temporary staging buffer of size " + GetFormattedSizeString(imgSize) + "\n");

	//Copy pixel data to staging buffer
	//Keep track of number of padding bytes for logging
	std::uint32_t numPaddingBytes{ 0 };
	VkCommandBuffer commandBuffer{ commandPool.AllocateCommandBuffer() };
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	TransitionImage(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, _arrSize, imageArray, &commandBuffer);
	for (std::size_t i{ 0 }; i < _arrSize; ++i)
	{
		//Load the data to disk
		if (_out_metadata != nullptr) { _out_metadata[i] = imageData[i].metadata; }
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Image " + std::to_string(i) + ": Loaded " + std::string(_filepaths[i]) + " from disk (" + std::to_string(imageData[i].metadata.width) + "x" + std::to_string(imageData[i].metadata.height) + ", " + std::to_string(imageData[i].metadata.channels) + " channels)\n");

		void* mappedMemory;
		vkMapMemory(device.GetDevice(), bufferFactory.GetMemory(stagingBuffer), 0, imgSize, 0, &mappedMemory);
		memcpy(mappedMemory, imageData[i].pixels, imageData[i].metadata.width * imageData[i].metadata.height * imageData[i].metadata.channels);
		vkUnmapMemory(device.GetDevice(), bufferFactory.GetMemory(stagingBuffer));
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Image " + std::to_string(i) + ": Pixel Data copied to staging buffer\n");

		//Free the image data as it's in the staging buffer now
		ImageLoader::Free(imageData[i].pixels);

		//Copy the staging buffer to the image
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = i;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { static_cast<std::uint32_t>(imageData[i].metadata.width), static_cast<std::uint32_t>(imageData[i].metadata.height), 1 };
		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, imageArray, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		const std::size_t currentNumPaddingBytes{ static_cast<std::size_t>((maxWidth - imageData[i].metadata.width) * (maxHeight - imageData[i].metadata.height) * (requiredNrChannels - imageData[i].metadata.channels)) };
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Image " + std::to_string(i) + ": " + GetFormattedSizeString(currentNumPaddingBytes) + " padding bytes added\n");
		numPaddingBytes += currentNumPaddingBytes;
	}
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  " + GetFormattedSizeString(numPaddingBytes) + " total padding bytes added\n");

	TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, _arrSize, imageArray, &commandBuffer);
	vkEndCommandBuffer(commandBuffer);

	//Execute the command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(device.GetGraphicsQueue());

	//Cleanup
	commandPool.FreeCommandBuffer(commandBuffer);
	bufferFactory.FreeBuffer(stagingBuffer);

	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Texture data successfully transferred to device local memory\n");

	return imageArray;
}



void ImageFactory::FreeImageImpl(VkImage& _image)
{
	if (imageMemoryMap[_image] != VK_NULL_HANDLE)
	{
		vkFreeMemory(device.GetDevice(), imageMemoryMap[_image], static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		imageMemoryMap[_image] = VK_NULL_HANDLE;
	}
	if (_image != VK_NULL_HANDLE)
	{
		vkDestroyImage(device.GetDevice(), _image, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
	}
	imageMemoryMap.erase(_image);
	_image = VK_NULL_HANDLE;
}



VkImageView ImageFactory::CreateImageViewImpl(const VkImage& _image, const VkFormat& _format, const VkImageAspectFlags& _aspectFlags, bool _arrayView, std::uint32_t _layerCount)
{
	//Create image view
	VkImageView imageView;

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = nullptr;
	viewInfo.flags = 0;
	viewInfo.image = _image;
	viewInfo.viewType = _arrayView ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D; //Todo: add more image types
	viewInfo.format = _format;
	viewInfo.subresourceRange.aspectMask = _aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = _layerCount;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Creating image view", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkCreateImageView(device.GetDevice(), &viewInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &imageView) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, "(" + std::to_string(result) + ")", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
	imageViewImageMap[imageView] = _image;

	return imageView;
}



void ImageFactory::FreeImageViewImpl(VkImageView& _imageView)
{
	if (_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(device.GetDevice(), _imageView, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
	}
	imageViewImageMap.erase(_imageView);
	_imageView = VK_NULL_HANDLE;
}



VkSampler ImageFactory::CreateSamplerImpl(const VkSamplerCreateInfo& _createInfo)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::IMAGE_FACTORY, "  Creating sampler", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkSampler sampler;
	VkResult result{ vkCreateSampler(device.GetDevice(), &_createInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &sampler) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::IMAGE_FACTORY, "(" + std::to_string(result) + ")", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
	samplers.push_back(sampler);

	return sampler;
}



void ImageFactory::FreeSamplerImpl(VkSampler& _sampler)
{
	if (_sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(device.GetDevice(), _sampler, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
	}
	std::erase(samplers, _sampler);
	_sampler = VK_NULL_HANDLE;
}



}