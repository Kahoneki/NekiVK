#ifndef IMAGEFACTORY_H
#define IMAGEFACTORY_H
#include "BufferFactory.h"
#include "../Utils/Loaders/ImageLoader.h"
#include "../Core/VulkanCommandPool.h"
#include "NekiVK/Utils/Loaders/ModelLoader.h"


//Responsible for the initialisation, ownership, and clean shutdown of VkImages and accompanying VkDeviceMemorys, VkImageViews, and VkSamplers
namespace Neki
{



class ImageFactory
{
public:
	explicit ImageFactory(const VKLogger& _logger,
	                      VKDebugAllocator& _deviceDebugAllocator,
	                      const VulkanDevice& _device,
	                      VulkanCommandPool& _commandPool,
	                      BufferFactory& _bufferFactory);

	~ImageFactory();


	//----IMAGES----//

	//Allocate a single image populated by data from _filepath on a device local heap (passed through an intermediate staging buffer)
	//Optionally, pass a _formatOverride to override the format chosen by default based on the image's channels
	//Optionally, pass a _textureType to automatically determine the appropriate format based on the provided texture type and the number of channels
	//Optionally, pass an ImageData pointer to get metadata about the image
	[[nodiscard]] VkImage AllocateImage(const char* _filepath, const VkImageUsageFlags _flags, VkFormat _formatOverride = VK_FORMAT_UNDEFINED, MODEL_TEXTURE_TYPE _textureType = MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES, bool _flipImage = false, ImageMetadata* _out_metadata = nullptr);

	//Allocate a single empty image on a device local heap (passed through an intermediate staging buffer)
	//Note: initial state is UNDEFINED - needs to be transitioned
	[[nodiscard]] VkImage AllocateImage(VkExtent2D _size, VkFormat _format, const VkImageUsageFlags _flags);

	//Allocate a vector of _count images populated by data from _filepaths on a device local heap (passed through intermediate staging buffers)
	//Optionally, pass a list of _count _formatOverrides to override the format chosen by default based on the image's channels
	//Optionally, pass a list of _count _textureTypes to automatically determine the appropriate formats based on the provided texture types and the number of channels
	//Optionally, pass a list of _count ImageDatas to get metadata about the images
	[[nodiscard]] std::vector<VkImage> AllocateImages(std::uint32_t _count, const char** _filepaths, const VkImageUsageFlags* _flags, VkFormat* _formatOverrides = nullptr, MODEL_TEXTURE_TYPE* _textureTypes = nullptr, bool* _flipImages = nullptr, ImageMetadata* _out_metadata = nullptr);

	//Allocate a vector of _count empty images on a device local heap (passed through intermediate staging buffers)
	//Note: initial state is UNDEFINED - needs to be transitioned
	[[nodiscard]] std::vector<VkImage> AllocateImages(std::uint32_t _count, const VkExtent2D* _sizes, const VkFormat* _formats, const VkImageUsageFlags* _flags, bool* _flipImages = nullptr);

	//Allocate an image array populated by data from _filepaths on a device local heap (passed through an intermediate staging buffer)
	//Optionally, pass a _formatOverride to override the format chosen by default based on the image's channels
	//Optionally, pass a _textureType to automatically determine the appropriate format based on the provided texture type and the number of channels
	//Optionally, pass an ImageData pointer to get metadata about the images
	[[nodiscard]] VkImage AllocateImageArray(std::uint32_t _arrSize, const char** _filepaths, const VkImageUsageFlags _flags, VkFormat _formatOverride = VK_FORMAT_UNDEFINED, MODEL_TEXTURE_TYPE _textureType = MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES, bool _flipImage = false, ImageMetadata* _out_metadata = nullptr);

	//Free a specific image
	void FreeImage(VkImage& _image);

	//Free a list of _count images
	void FreeImages(std::uint32_t _count, VkImage* _images);


	//Transition an image from one state to another
	//Optionally, pass a (already begun) command buffer to this function and the barrier command will be recorded to it but not executed
	//Leaving _commandBuffer as nullptr will cause the function to allocate its own command buffer and automatically submit it
	void TransitionImage(VkImageLayout _srcLayout, VkImageLayout _dstLayout,
	                     VkImageAspectFlags _aspectMask,
	                     VkAccessFlags _srcAccessMask, VkAccessFlags _dstAccessMask,
	                     VkPipelineStageFlags _srcStageMask, VkPipelineStageFlags _dstStageMask,
	                     std::size_t layers,
	                     VkImage& _image,
	                     VkCommandBuffer* _commandBuffer = nullptr);

	//--------//



	//----IMAGE VIEWS----//

	//Create a single image view for _image of format _format
	[[nodiscard]] VkImageView CreateImageView(const VkImage& _image, const VkFormat& _format, const VkImageAspectFlags& _aspectFlags, bool _arrayView = false, std::uint32_t _layerCount = 1);

	//Create a vector of _count image views for _images of formats _formats
	[[nodiscard]] std::vector<VkImageView> CreateImageViews(std::uint32_t _count, const VkImage* _images, const VkFormat* _formats, const VkImageAspectFlags* _aspectFlags, bool* _arrayViews = nullptr, std::uint32_t* _layerCounts = nullptr);

	//Free a specific image view
	void FreeImageView(VkImageView& _imageView);

	//Free a list of _count image views
	void FreeImageViews(std::uint32_t _count, VkImageView* _imageViews);

	//--------//



	//----SAMPLERS----//

	//Create a single sampler
	[[nodiscard]] VkSampler CreateSampler(const VkSamplerCreateInfo& _createInfo);

	//Create a vector of _count samplers
	[[nodiscard]] std::vector<VkSampler> CreateSamplers(std::uint32_t _count, const VkSamplerCreateInfo* _createInfos);

	//Free a specific sampler
	void FreeSampler(VkSampler& _sampler);

	//Free a list of _count samplers
	void FreeSampler(std::uint32_t _count, VkSampler* _samplers);

	//--------//


private:
	[[nodiscard]] static VkFormat ChooseFormat(int _nrChannels, VkFormat _formatOverride, MODEL_TEXTURE_TYPE _textureType);

	[[nodiscard]] VkImage AllocateImageImpl(const char* _filepath, VkImageUsageFlags _flags, VkFormat _formatOverride, MODEL_TEXTURE_TYPE _textureType, bool _flipImage, ImageMetadata* _out_metadata);
	[[nodiscard]] VkImage AllocateImageImpl(VkExtent2D _size, VkFormat _format, const VkImageUsageFlags _flags, std::size_t _layers = 1);
	[[nodiscard]] VkImage AllocateImageArrayImpl(std::uint32_t _arrSize, const char** _filepaths, VkImageUsageFlags _flags, VkFormat _formatOverride, MODEL_TEXTURE_TYPE _textureType, bool _flipImage, ImageMetadata* _out_metadata);
	void FreeImageImpl(VkImage& _image);

	[[nodiscard]] VkImageView CreateImageViewImpl(const VkImage& _image, const VkFormat& _format, const VkImageAspectFlags& _flags, bool _arrayView, std::uint32_t _layerCount);
	void FreeImageViewImpl(VkImageView& _imageView);

	[[nodiscard]] VkSampler CreateSamplerImpl(const VkSamplerCreateInfo& _createInfo);
	void FreeSamplerImpl(VkSampler& _sampler);

	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;
	BufferFactory& bufferFactory;
	VulkanCommandPool& commandPool;

	std::unordered_map<VkImage, VkDeviceMemory> imageMemoryMap;
	std::unordered_map<VkImageView, VkImage> imageViewImageMap;
	std::vector<VkSampler> samplers;
};



}



#endif