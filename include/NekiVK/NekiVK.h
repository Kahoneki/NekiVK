#ifndef NEKIVK_H
#define NEKIVK_H



//Include all header files - allowing user to #include <NekiVK/NekiVK.h> to include the entire library

#include "Core/VulkanCommandPool.h"
#include "Core/VulkanDescriptorPool.h"
#include "Core/VulkanDevice.h"
#include "Core/VulkanGraphicsPipeline.h"
#include "Core/VulkanPipeline.h"
#include "Core/VulkanRenderManager.h"
#include "Core/VulkanSwapchain.h"

#include "Debug/VKLogger.h"
#include "Debug/VKDebugAllocator.h"
#include "Debug/VKLoggerConfig.h"

#include "Memory/BufferFactory.h"
#include "Memory/ImageFactory.h"

#include "Utils/Loaders/ImageLoader.h"
#include "Utils/Strings/format.h"
#include "Utils/Templates/enum_enable_bitmask_operators.h"



#endif