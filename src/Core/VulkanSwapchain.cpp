#include "NekiVK/Core/VulkanSwapchain.h"
#include <stdexcept>
#include <algorithm>

namespace Neki
{



VulkanSwapchain::VulkanSwapchain(const VKLogger& _logger, VKDebugAllocator& _deviceDebugAllocator, const VulkanDevice& _device, ImageFactory& _imageFactory, VkExtent2D _windowSize)\
								: logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device), imageFactory(_imageFactory)
{
	windowSize = _windowSize;
	window = nullptr;
	surface = VK_NULL_HANDLE;
	swapchain = VK_NULL_HANDLE;
	swapchainImageFormat = VK_FORMAT_UNDEFINED;

	if (windowSize.height == 0 || windowSize.width == 0)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::SWAPCHAIN, "Provided _windowSize (" + std::to_string(windowSize.width) + "x" + std::to_string(windowSize.height) + ") has a zero-length dimension.");
		throw std::runtime_error("");
	}

	CreateWindow();
	CreateSurface();
	CreateSwapchain();
	CreateSwapchainImageViews();
}



VulkanSwapchain::~VulkanSwapchain()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::SWAPCHAIN,"Shutting down VulkanSwapchain\n");

	if (!swapchainImageViews.empty())
	{
		for (VkImageView v : swapchainImageViews)
		{
			vkDestroyImageView(device.GetDevice(), v, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::SWAPCHAIN,"  Swapchain Image Views Destroyed\n");
	}
	swapchainImageViews.clear();

	if (swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device.GetDevice(), swapchain, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		swapchain = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::SWAPCHAIN,"  Swapchain Destroyed\n");
	}

	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(device.GetInstance(), surface, nullptr); //GLFW does its own allocations which gets buggy when combined with VkAllocationCallbacks, don't use debug allocator
		surface = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::SWAPCHAIN,"  Surface Destroyed\n");
	}

	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::SWAPCHAIN,"  Window Destroyed\n");
	}
}



GLFWwindow* VulkanSwapchain::GetWindow()
{
	return window;
}



bool VulkanSwapchain::WindowShouldClose() const
{
	return glfwWindowShouldClose(window);
}



VkFormat VulkanSwapchain::GetSwapchainFormat() const
{
	return swapchainImageFormat;
}



VkExtent2D VulkanSwapchain::GetSwapchainExtent() const
{
	return swapchainExtent;
}



std::size_t VulkanSwapchain::GetSwapchainSize() const
{
	return swapchainImages.size();
}



VkSwapchainKHR VulkanSwapchain::GetSwapchain()
{
	return swapchain;
}



VkImageView VulkanSwapchain::GetSwapchainImageView(std::size_t _index)
{
	return swapchainImageViews[_index];
}



void VulkanSwapchain::CreateWindow()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::SWAPCHAIN, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::SWAPCHAIN, "Creating Window\n");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Don't create an OpenGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //Don't let window be resized (for now)
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Creating window", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	window = glfwCreateWindow(windowSize.width, windowSize.height, "Vulkaneki", nullptr, nullptr);
	logger.Log(window ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::SWAPCHAIN, window ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
	if (!window)
	{
		throw std::runtime_error("");
	}
}



void VulkanSwapchain::CreateSurface()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::SWAPCHAIN, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::SWAPCHAIN, "Creating Surface\n");
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Creating surface", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ glfwCreateWindowSurface(device.GetInstance(), window, nullptr, &surface) }; //GLFW does its own allocations which gets buggy when combined with VkAllocationCallbacks, don't use debug allocator
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::SWAPCHAIN, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::SWAPCHAIN, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
}



void VulkanSwapchain::CreateSwapchain()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::SWAPCHAIN, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::SWAPCHAIN, "Creating Swapchain\n");


	//Get supported formats
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Scanning for supported swapchain formats", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	
	//Query surface capabilities
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetPhysicalDevice(), surface, &capabilities);

	//Query supported formats
	std::uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats;
	if (formatCount != 0)
	{
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), surface, &formatCount, formats.data());
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::SWAPCHAIN, "Found: " + std::to_string(formatCount) + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	for (const VkSurfaceFormatKHR& format : formats)
	{
		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "  Format: " + std::to_string(format.format) + " (Colour Space: " + std::to_string(format.colorSpace) + ")\n");
	}

	//Choose suitable format
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Selecting swapchain format (preferred: " + std::to_string(VK_FORMAT_B8G8R8A8_SRGB) + " - VK_FORMAT_B8G8R8A8_SRGB\n");
	VkSurfaceFormatKHR surfaceFormat{ formats[0] }; //Default to first one if ideal isn't found
	for (const VkSurfaceFormatKHR& availableFormat : formats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceFormat = availableFormat;
			break;
		}
	}
	swapchainImageFormat = surfaceFormat.format;
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		swapchainExtent = capabilities.currentExtent;
	}
	else
	{
		swapchainExtent.width = std::clamp(static_cast<std::uint32_t>(windowSize.width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		swapchainExtent.height = std::clamp(static_cast<std::uint32_t>(windowSize.height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Selected swapchain format " + std::to_string(swapchainImageFormat) + "\n");
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Selected swapchain extent " + std::to_string(swapchainExtent.width) + "x" + std::to_string(swapchainExtent.height) + "\n");


	//Get supported present modes
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Scanning for supported swapchain present modes", VK_LOGGER_WIDTH::SUCCESS_FAILURE);

	//Query supported present modes
	std::uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device.GetPhysicalDevice(), surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes;
	if (presentModeCount != 0)
	{
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device.GetPhysicalDevice(), surface, &presentModeCount, presentModes.data());
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::SWAPCHAIN, "Found: " + std::to_string(presentModeCount) + "\n", VK_LOGGER_WIDTH::DEFAULT, false);
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::SWAPCHAIN, "  ", VK_LOGGER_WIDTH::DEFAULT, false);
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::SWAPCHAIN, "Present mode: " + std::to_string(presentMode) + " (" + (presentMode == 0 ? "IMMEDIATE)\n" :
																																			(presentMode == 1 ? "MAILBOX)\n" :
																																			(presentMode == 2 ? "FIFO)\n" :
																																			(presentMode == 3 ? "FIFO_RELAXED\n" : ")\n")))));
	}
	
	//Choose suitable present mode
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Selecting swapchain present mode (preferred: " + std::to_string(VK_PRESENT_MODE_MAILBOX_KHR) + " - VK_PRESENT_MODE_MAILBOX_KHR)\n");
	VkPresentModeKHR presentMode{ VK_PRESENT_MODE_FIFO_KHR }; //Guaranteed to be available
	for (const VkPresentModeKHR& availablePresentMode : presentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			presentMode = availablePresentMode;
			break;
		}
	}
	logger.Log(presentMode == VK_PRESENT_MODE_MAILBOX_KHR ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::WARNING, VK_LOGGER_LAYER::SWAPCHAIN, "Selected swapchain present mode: " + std::to_string(presentMode) + "\n");


	//Get image count
	std::uint32_t imageCount{ capabilities.minImageCount + 1 }; //Get 1 more than minimum required by driver for a bit of wiggle room
	if (capabilities.maxImageCount >0 && imageCount > capabilities.maxImageCount) { imageCount = capabilities.maxImageCount; }
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Requesting " + std::to_string(imageCount) + " swapchain images\n");

	
	//Create swapchain
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = swapchainImageFormat;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapchainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Creating swapchain", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result = vkCreateSwapchainKHR(device.GetDevice(), &createInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &swapchain);
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::SWAPCHAIN, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result == VK_SUCCESS)
	{
		//Get number of images in swapchain
		vkGetSwapchainImagesKHR(device.GetDevice(), swapchain, &imageCount, nullptr);
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::SWAPCHAIN, " (images: " + std::to_string(imageCount) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
	}
	else
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::SWAPCHAIN, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}


	//Get handles to swapchain's images
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device.GetDevice(), swapchain, &imageCount, swapchainImages.data());
}



void VulkanSwapchain::CreateSwapchainImageViews()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::SWAPCHAIN, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::SWAPCHAIN, "Creating Swapchain Image Views\n");

	swapchainImageViews.resize(swapchainImages.size());
	for (std::size_t i{ 0 }; i < swapchainImages.size(); ++i)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.image = swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::SWAPCHAIN, "Creating image view " + std::to_string(i), VK_LOGGER_WIDTH::SUCCESS_FAILURE);
		VkResult result = vkCreateImageView(device.GetDevice(), &createInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &swapchainImageViews[i]);
		logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::SWAPCHAIN, result == VK_SUCCESS ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
		if (result != VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::SWAPCHAIN, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
			throw std::runtime_error("");
		}
	}
}



}