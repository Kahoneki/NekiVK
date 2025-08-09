#include "NekiVK/Core/VulkanRenderManager.h"
#include "NekiVK/Debug/VKLogger.h"

#include <stdexcept>
#include <algorithm>


namespace Neki
{



VulkanRenderManager::VulkanRenderManager(const VKLogger& _logger, VKDebugAllocator& _deviceDebugAllocator, const VulkanDevice& _device, VulkanSwapchain& _swapchain, ImageFactory& _imageFactory, VulkanCommandPool& _commandPool, std::size_t _framesInFlight, VKRenderPassCleanDesc _renderPassDesc)
										: logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device), swapchain(_swapchain), imageFactory(_imageFactory), commandPool(_commandPool)
{
	renderPass = VK_NULL_HANDLE;
	framesInFlight = _framesInFlight;
	imageIndex = 0;
	currentFrame = 0;
	
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Render Manager\n");

	defaultDepthTextureFormat = device.FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	
	AllocateCommandBuffers();
	CreateRenderPass(_renderPassDesc);
	CreateSwapchainFramebuffers(_renderPassDesc);
	CreateSyncObjects();
}



VulkanRenderManager::~VulkanRenderManager()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER,"Shutting down VulkanRenderManager\n");

	if (!inFlightFences.empty())
	{
		for (VkFence& f : inFlightFences)
		{
			if (f != VK_NULL_HANDLE)
			{
				vkDestroyFence(device.GetDevice(), f, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
			}
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  In-Flight Fences Destroyed\n");
	}
	inFlightFences.clear();
	imagesInFlight.clear();

	if (!renderFinishedSemaphores.empty())
	{
		for (VkSemaphore& s : renderFinishedSemaphores)
		{
			if (s != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(device.GetDevice(), s, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
			}
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Render-Finished Semaphores Destroyed\n");
	}
	renderFinishedSemaphores.clear();

	if (!imageAvailableSemaphores.empty())
	{
		for (VkSemaphore& s : imageAvailableSemaphores)
		{
			if (s != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(device.GetDevice(), s, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
			}
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Image-Available Semaphores Destroyed\n");
	}
	imageAvailableSemaphores.clear();
	
	if (!swapchainFramebuffers.empty())
	{
		for (VkFramebuffer f : swapchainFramebuffers)
		{
			vkDestroyFramebuffer(device.GetDevice(), f, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Swapchain Framebuffers Destroyed\n");
	}
	swapchainFramebuffers.clear();
	


	if (renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(device.GetDevice(), renderPass, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		renderPass = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER,"  Render Pass Destroyed\n");
	}


}



VkAttachmentDescription VulkanRenderManager::GetDefaultOutputColourAttachmentDescription()
{
	VkAttachmentDescription colourAttachment;
	colourAttachment.flags = 0;
	colourAttachment.format = VK_FORMAT_UNDEFINED;
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //No MSAA
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //Colour attachment - no stencil
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //Colour attachment - no stencil
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //Layout to transition state after pass is done (for presentation)

	return colourAttachment;
}



VkAttachmentDescription VulkanRenderManager::GetDefaultOutputDepthAttachmentDescription()
{
	VkAttachmentDescription depthAttachment;
	depthAttachment.flags = 0;
	depthAttachment.format = VK_FORMAT_UNDEFINED;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //No MSAA
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //Don't need depth after the frame
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	return depthAttachment;
}



VkSubpassDescription VulkanRenderManager::GetDefaultSubpassDescription(VkAttachmentReference* _colourAttachments, VkAttachmentReference* _depthStencilAttachment)
{
	VkSubpassDescription subpassDescription{};
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = _colourAttachments;
	subpassDescription.pDepthStencilAttachment = _depthStencilAttachment;

	return subpassDescription;
}



void VulkanRenderManager::StartFrame(std::uint32_t _clearValueCount, const VkClearValue* _clearValues)
{
	//Wait for previous rendering for the frame of this frame index to finish before overwriting the command buffer for the frame
	vkWaitForFences(device.GetDevice(), 1, &(inFlightFences[currentFrame]), VK_TRUE, UINT64_MAX);

	//Get index of the next image in the swapchain and pass a semaphore to be signalled once the image is available (no longer being read for presentation)
	vkAcquireNextImageKHR(device.GetDevice(), swapchain.GetSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	//Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(device.GetDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	
	//Mark the image as now being in use by this frame - tying its synchronisation to this frame's fence
	//The next frame that wants to use this image will have to wait for this frame to finish rendering to it so as to not overwrite
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];
	
	//Reset back to unsignalled - fence will be signalled again once rendering of this frame has finished
	vkResetFences(device.GetDevice(), 1, &(inFlightFences[currentFrame]));

	
	//Start recording the command buffer
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;
	if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Failed to begin command buffer\n");
		throw std::runtime_error("");
	}

	
	//Define the render pass begin info
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapchain.GetSwapchainExtent();
	renderPassInfo.clearValueCount = _clearValueCount;
	renderPassInfo.pClearValues = _clearValues;
	
	vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}



void VulkanRenderManager::SubmitAndPresent()
{
	//Stop recording commands
	vkCmdEndRenderPass(commandBuffers[currentFrame]);
	if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Failed to end command buffer\n");
		throw std::runtime_error("");
	}


	//Submit command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;

	//Todo: get wait stages from render pass create info
	constexpr VkPipelineStageFlags waitStages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &(imageAvailableSemaphores[currentFrame]);
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &(renderFinishedSemaphores[imageIndex]);

	if (vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Failed to submit command buffer\n");
		throw std::runtime_error("");
	}


	//Present the result
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &(renderFinishedSemaphores[imageIndex]);
	presentInfo.swapchainCount = 1;
	VkSwapchainKHR underlyingSwapchain{ swapchain.GetSwapchain() };
	presentInfo.pSwapchains = &underlyingSwapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(device.GetGraphicsQueue(), &presentInfo);
	
	
	currentFrame = (currentFrame + 1) % framesInFlight;
}



VkCommandBuffer VulkanRenderManager::GetCurrentCommandBuffer()
{
	return commandBuffers[currentFrame];
}


VkRenderPass VulkanRenderManager::GetRenderPass()
{
	return renderPass;
}



VkImageView VulkanRenderManager::GetFramebufferImageView(std::size_t _index)
{
	return framebufferImageViews[_index];
}



void VulkanRenderManager::AllocateCommandBuffers()
{
	commandBuffers.resize(framesInFlight);
	commandBuffers = commandPool.AllocateCommandBuffers(framesInFlight);
}



void VulkanRenderManager::CreateRenderPass(VKRenderPassCleanDesc& _renderPassDesc)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Render Pass\n");
	
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.dependencyCount = _renderPassDesc.dependencyCount;
	renderPassCreateInfo.pDependencies = _renderPassDesc.dependencies;
	renderPassCreateInfo.attachmentCount = _renderPassDesc.attachmentCount;
	renderPassCreateInfo.pAttachments = _renderPassDesc.attachments;
	renderPassCreateInfo.subpassCount = _renderPassDesc.subpassCount;
	renderPassCreateInfo.pSubpasses = _renderPassDesc.subpasses;
	
	//Loop through pAttachments and check format - if UNDEFINED, set to swapchain image format or depth format (depending on type)
	//Can't modify original array, so remake it
	std::vector<VkAttachmentDescription> attachments;
	if (renderPassCreateInfo.pAttachments != nullptr)
	{
		attachments.assign(renderPassCreateInfo.pAttachments, renderPassCreateInfo.pAttachments + renderPassCreateInfo.attachmentCount);
	}
	for (std::size_t i{ 0 }; i<attachments.size(); ++i)
	{
		if (_renderPassDesc.attachmentFormats[i] == VK_FORMAT_UNDEFINED)
		{
			FORMAT_TYPE type{ _renderPassDesc.attachmentTypes[i] };
			if (type == FORMAT_TYPE::SWAPCHAIN || type == FORMAT_TYPE::COLOUR_SAMPLED || type == FORMAT_TYPE::COLOUR_INPUT_ATTACHMENT)
			{
				logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Setting attachment " + std::to_string(i) + " to swapchain image format\n");
				attachments[i].format = swapchain.GetSwapchainFormat();
			}
			else if (type == FORMAT_TYPE::DEPTH_NO_SAMPLING || type == FORMAT_TYPE::DEPTH_SAMPLED || type == FORMAT_TYPE::DEPTH_INPUT_ATTACHMENT)
			{
				logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Setting attachment " + std::to_string(i) + " to depth format\n");
				attachments[i].format = defaultDepthTextureFormat;
			}
		}
	}
	renderPassCreateInfo.pAttachments = attachments.data();

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating render pass", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result = vkCreateRenderPass(device.GetDevice(), &renderPassCreateInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &renderPass);
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
}



void VulkanRenderManager::CreateSwapchainFramebuffers(const VKRenderPassCleanDesc& _renderPassDesc)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Swapchain Framebuffers\n");

	//Final attachment must be swapchain
	if (_renderPassDesc.attachmentTypes[_renderPassDesc.attachmentCount-1] != FORMAT_TYPE::SWAPCHAIN)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Final image in framebuffer description must be of type SWAPCHAIN");
		throw std::runtime_error("");
	}
	//Final attachment must be VK_FORMAT_UNDEFINED as per the Neki standard - enforcing this prevents accidents
	if (_renderPassDesc.attachmentFormats[_renderPassDesc.attachmentCount-1] != VK_FORMAT_UNDEFINED)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Final image in framebuffer description must be of format UNDEFINED");
	}

	//Create images and image views based on provided description
	//Ignore last attachment because the Neki standard states it must be the swapchain - swapchain image is populated further down
	for (std::size_t i{ 0 }; i<_renderPassDesc.attachmentCount-1; ++i)
	{
		const FORMAT_TYPE formatType{ _renderPassDesc.attachmentTypes[i] };
		VkFormat format{ _renderPassDesc.attachmentFormats[i] };
		if (format == VK_FORMAT_UNDEFINED)
		{
			const bool useSwapchainFormat{ formatType == FORMAT_TYPE::SWAPCHAIN || formatType == FORMAT_TYPE::COLOUR_INPUT_ATTACHMENT || formatType == FORMAT_TYPE::COLOUR_SAMPLED };
			format = (useSwapchainFormat) ? swapchain.GetSwapchainFormat() : defaultDepthTextureFormat;
		}

		if (formatType == FORMAT_TYPE::COLOUR_INPUT_ATTACHMENT || formatType == FORMAT_TYPE::COLOUR_SAMPLED)
		{
			const VkImageUsageFlagBits flag{ (formatType == FORMAT_TYPE::COLOUR_INPUT_ATTACHMENT) ? VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT : VK_IMAGE_USAGE_SAMPLED_BIT };
			framebufferImages.push_back(imageFactory.AllocateImage(swapchain.GetSwapchainExtent(), format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | flag));
			framebufferImageViews.push_back(imageFactory.CreateImageView(framebufferImages[i], format, VK_IMAGE_ASPECT_COLOR_BIT));
		}
		else if (formatType == FORMAT_TYPE::DEPTH_NO_SAMPLING || formatType == FORMAT_TYPE::DEPTH_INPUT_ATTACHMENT || formatType == FORMAT_TYPE::DEPTH_SAMPLED)
		{
			if (formatType == FORMAT_TYPE::DEPTH_NO_SAMPLING)
			{
				framebufferImages.push_back(imageFactory.AllocateImage(swapchain.GetSwapchainExtent(), format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));
			}
			else
			{
				const VkImageUsageFlagBits flag{ (formatType == FORMAT_TYPE::DEPTH_INPUT_ATTACHMENT) ? VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT : VK_IMAGE_USAGE_SAMPLED_BIT };
				framebufferImages.push_back(imageFactory.AllocateImage(swapchain.GetSwapchainExtent(), format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | flag));	
			}
			framebufferImageViews.push_back(imageFactory.CreateImageView(framebufferImages[i], format, VK_IMAGE_ASPECT_DEPTH_BIT));
		}
	}

	//Create a framebuffer for each swapchain image
	//Each framebuffer should contain all the provided attachments as well as the corresponding swapchain image
	swapchainFramebuffers.resize(swapchain.GetSwapchainSize());
	for (std::size_t i{ 0 }; i < swapchainFramebuffers.size(); ++i)
	{
		framebufferImageViews.push_back(swapchain.GetSwapchainImageView(i));
		
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext = nullptr;
		framebufferInfo.flags = 0;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = _renderPassDesc.attachmentCount;
		framebufferInfo.pAttachments = framebufferImageViews.data();
		framebufferInfo.width = swapchain.GetSwapchainExtent().width;
		framebufferInfo.height = swapchain.GetSwapchainExtent().height;
		framebufferInfo.layers = 1;

		logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating framebuffer " + std::to_string(i), VK_LOGGER_WIDTH::SUCCESS_FAILURE);
		VkResult result = vkCreateFramebuffer(device.GetDevice(), &framebufferInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &swapchainFramebuffers[i]);
		logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, result == VK_SUCCESS ? "success\n" : "failure\n", VK_LOGGER_WIDTH::DEFAULT, false);
		if (result != VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, " (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
			throw std::runtime_error("");
		}

		//Remove swapchain image view for next pass
		framebufferImageViews.pop_back();
	}
}



void VulkanRenderManager::CreateSyncObjects()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::RENDER_MANAGER, "Creating Sync Objects\n");

	//Allow for MAX_FRAMES_IN_FLIGHT frames to be in-flight at once
	imageAvailableSemaphores.resize(framesInFlight);
	renderFinishedSemaphores.resize(swapchain.GetSwapchainSize());
	inFlightFences.resize(framesInFlight);
	imagesInFlight.resize(swapchain.GetSwapchainSize());

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (std::size_t i{ 0 }; i<swapchain.GetSwapchainSize(); ++i)
	{
		if (vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &renderFinishedSemaphores[i]) != VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Failed to create per-image sync objects\n");
			throw std::runtime_error("");
		}
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER, "Successfully created sync objects for " + std::to_string(swapchain.GetSwapchainSize()) + " images\n");
	for (std::size_t i{ 0 }; i<framesInFlight; ++i)
	{
		if (vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device.GetDevice(), &fenceInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &inFlightFences[i]) != VK_SUCCESS)
		{
			logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::RENDER_MANAGER, "Failed to create per-frame sync objects\n");
			throw std::runtime_error("");
		}
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::RENDER_MANAGER, "Successfully created sync objects for " + std::to_string(framesInFlight) + " frames\n");
}



}