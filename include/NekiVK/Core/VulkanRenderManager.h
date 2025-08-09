#ifndef VULKANRENDERMANAGER_H
#define VULKANRENDERMANAGER_H

#include "VulkanCommandPool.h"
#include "VulkanSwapchain.h"
#include "../Memory/ImageFactory.h"


//Responsible for the initialisation, ownership, and clean shutdown of a GLFWwindow, VkSurfaceKHR, VkSwapchainKHR, VkImage (depthTexture), VkImageView (depthTextureView), VkRenderPass, and all accompanying sync objects
namespace Neki
{

//Helper enum for VKRenderPassCleanDesc and VKFramebufferCleanDesc
enum class FORMAT_TYPE
{
	SWAPCHAIN,

	//Adds the VK_IMAGE_USAGE_COLOR_INPUT_ATTACHMENT flag - faster but doesn't have support for arbitrary pixel sampling (e.g.: for neighbour sampling effects)
	COLOUR_INPUT_ATTACHMENT,

	//Adds the VK_IMAGE_USAGE_INPUT_ATTACHMENT flag - slower but has support for arbitrary pixel sampling (e.g.: for neighbour sampling effects)
	COLOUR_SAMPLED,

	//Used for depth testing only - sampling in shaders won't be permitted
	DEPTH_NO_SAMPLING,
	
	//Adds the VK_IMAGE_USAGE_COLOR_INPUT_ATTACHMENT flag - faster but doesn't have support for arbitrary pixel sampling (e.g.: for neighbour sampling effects)
	DEPTH_INPUT_ATTACHMENT,

	//Adds the VK_IMAGE_USAGE_INPUT_ATTACHMENT flag - slower but has support for arbitrary pixel sampling (e.g.: for neighbour sampling effects)
	DEPTH_SAMPLED,
};

//Passed to constructor
struct VKRenderPassCleanDesc final
{
	//Upon passing to VulkanRenderManager's constructor, attachmentCount attachments will be created with the corresponding attachmentFormats
	//To use swapchain image format for intermediate textures, set the appropriate attachmentFormat to VK_FORMAT_UNDEFINED and attachmentType to COLOUR_X
	//To use default depth format, set the appropriate attachmentFormat to VK_FORMAT_UNDEFINED and attachmentType to DEPTH_X
	//To specify the attachment which is to be populated by the swapchain image, set attachmentFormat to VK_FORMAT_UNDEFINED and attachmentType to SWAPCHAIN
	//Final attachment must be swapchain
	//To use any other format, set the appropriate attachmentFormat to the desired VkFormat and attachmentType to the corresponding type (COLOUR_X or DEPTH_X)
	std::uint32_t attachmentCount;
	VkAttachmentDescription* attachments;
	VkFormat* attachmentFormats;
	FORMAT_TYPE* attachmentTypes;

	std::uint32_t subpassCount;
	VkSubpassDescription* subpasses;

	std::uint32_t dependencyCount;
	VkSubpassDependency* dependencies;
};


class VulkanRenderManager
{
public:
	explicit VulkanRenderManager(const VKLogger& _logger,
							 VKDebugAllocator& _deviceDebugAllocator,
							 const VulkanDevice& _device,
							 VulkanSwapchain& _swapchain,
							 ImageFactory& _imageFactory,
							 VulkanCommandPool& _commandPool,
							 std::size_t _framesInFlight,
							 VKRenderPassCleanDesc _renderPassDesc);

	~VulkanRenderManager();

	//Descriptions for a single-subpass render pass
	[[nodiscard]] static VkAttachmentDescription GetDefaultOutputColourAttachmentDescription();
	[[nodiscard]] static VkAttachmentDescription GetDefaultOutputDepthAttachmentDescription();
	[[nodiscard]] static VkSubpassDescription GetDefaultSubpassDescription(VkAttachmentReference* _colourAttachments, VkAttachmentReference* _depthStencilAttachment);
	
	void StartFrame(std::uint32_t _clearValueCount, const VkClearValue* _clearValues);
	void SubmitAndPresent();

	[[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer();
	[[nodiscard]] VkRenderPass GetRenderPass();
	[[nodiscard]] VkImageView GetFramebufferImageView(std::size_t _index);

	
private:
	void AllocateCommandBuffers();
	void CreateRenderPass(VKRenderPassCleanDesc& _renderPassDesc);
	void CreateSwapchainFramebuffers(const VKRenderPassCleanDesc& _renderPassDesc);
	void CreateSyncObjects();

	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;
	ImageFactory& imageFactory;
	VulkanSwapchain& swapchain;
	VulkanCommandPool& commandPool;

	VkFormat defaultDepthTextureFormat;
	VkRenderPass renderPass;

	//For framebuffer
	std::vector<VkImage> framebufferImages; //Contains all non-swapchain images
	std::vector<VkImageView> framebufferImageViews;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	
	//Sync objects
	std::vector<VkSemaphore> imageAvailableSemaphores; //imageAvailableSemaphores[currentFrame] signalled when the image for frame currentFrame has finished being presented and can start being overwritten again
	std::vector<VkSemaphore> renderFinishedSemaphores; //renderFinishedSemaphores[imageIndex] signalled when rendering has finished for image imageIndex
	std::vector<VkFence> inFlightFences; //inFlightFences[currentFrame] signalled when frame currentFrame has finished rendering to the image (commandBuffers[currentFrame] can be overwritten)
	std::vector<VkFence> imagesInFlight; //imagesInFlight[imageIndex] signalled when image imageIndex has finished being rendered to (tied to inFlightFences[x] where x is the frame the image is used in)
	std::uint32_t imageIndex; //The index of the image to be rendered to on the current frame (acquired from the swapchain)
	std::size_t currentFrame; //The index of the current frame in flight to be rendered to
	std::size_t framesInFlight; //The total number of frames in flight (currentFrame = (0, framesInFlight])
	
	std::vector<VkCommandBuffer> commandBuffers;
};
}

#endif