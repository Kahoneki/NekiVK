#ifndef VULKANSWAPCHAIN_H
#define VULKANSWAPCHAIN_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../Memory/ImageFactory.h"



namespace Neki
{



class VulkanSwapchain
{
public:
	explicit VulkanSwapchain(const VKLogger& _logger,
							 VKDebugAllocator& _deviceDebugAllocator,
							 const VulkanDevice& _device,
							 ImageFactory& _imageFactory,
							 VkExtent2D _windowSize);

	~VulkanSwapchain();

	[[nodiscard]] GLFWwindow* GetWindow();
	[[nodiscard]] bool WindowShouldClose() const;
	[[nodiscard]] VkFormat GetSwapchainFormat() const;
	[[nodiscard]] VkExtent2D GetSwapchainExtent() const;
	[[nodiscard]] std::size_t GetSwapchainSize() const; //Returns number of images in swapchain
	[[nodiscard]] VkSwapchainKHR GetSwapchain();
	[[nodiscard]] VkImageView GetSwapchainImageView(std::size_t _index);
	
	
private:
	void CreateWindow();
	void CreateSurface();
	void CreateSwapchain();
	void CreateSwapchainImageViews();

	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;
	ImageFactory& imageFactory;
	
	VkExtent2D windowSize;
	GLFWwindow* window;
	VkSurfaceKHR surface;

	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
};



}



#endif
