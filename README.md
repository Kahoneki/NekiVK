# NekiVK - Foundational Backend Vulkan Toolkit

NekiVK is a modern C++23 utility toolkit designed to accelerate the development of Vulkan applications. It encapsulates some of the more verbose and boilerplate-heavy aspects of the Vulkan API into a set of a clean, reusable, and self-managing (RAII) C++ classes. It aims to accomplish this while still providing the full feature set and configurability of the underlying API.

This library primarily serves as a personal learning project of mine in an effort to develop a deep understanding for modern graphics APIs, professional software architecture, and build systems. In this sense, it's built to prioritise clarity and flexibility over enforcing a rigid engine structure.

![Language](https://img.shields.io/badge/Language-C++23-pink.svg)
![CMake](https://img.shields.io/badge/CMake-3.28+-pink.svg)
![License](https://img.shields.io/badge/License-MIT-pink.svg)

<br></br>
## Features
*  **RAII-Compliant Wrappers:** Manages the lifetime of core Vulkan objects (`VkDevice`, `VkSwapchain`, `VkCommandPool`, etc.) automatically through the RAII design pattern.
*  **Simplified Resource Management**: Provides `BufferFactory` and `ImageFactory` classes to handle the complexities of allocating, binding, and data transfer for buffers and images.
*  **Flexible Architecture:** Built to provide the foundational building blocks for the majority of use-cases without enforcing a specific render loop or engine design.
*  **Modern CMake Integration:** Designed as a static library to be added to any C++23 project using CMake's `FetchContent` module. Also handles automatic dependency propagation (Vulkan, GLFW, GLM).

<br></br>
## Usage
NekiVK is intended to be added as a dependency through CMake's `FetchContent` module. To include NekiVK in your project, add the following snippet to your `CMakeLists.txt`:
```cmake
include(FetchContent)
FetchContent_Declare(
  NekiVK
  GIT_REPOSITORY "https://github.com/Kahoneki/NekiVK.git"
  GIT_TAG "v1.0.0" #It's recommended to use a specific release tag for stability (format: v[MAJOR].[MINOR].[PATCH]).
)
FetchContent_MakeAvailable(NekiVK)
target_link_libraries(EXECUTABLE_NAME PRIVATE NekiVK)
```
Note: The Vulkan SDK is a prerequisite for working with NekiVK. Download here:  
- Windows / Linux (Debian/Ubuntu) / MacOS - https://vulkan.lunarg.com/  
- Arch - AUR Packages: vulkan-headers, vulkan-validation-layers, vulkan-man-pages, vulkan-tools  

The classes can then be included in your application code with `#include <NekiVK/NekiVK.h>`
Example:
```cpp
#include <NekiVK/NekiVK.h>

int main()
{
  //The application is responsible for creating and handling the library's components
  //Lifetime management of underlying Vulkan resources internal to the classes is handled by RAII
  std::unique_ptr<Neki::VulkanDevice> device{ std::make_unique<Neki::VulkanDevice>(...) };
  std::unique_ptr<Neki::VulkanCommandPool> commandPool{ std::make_unique<Neki::VulkanCommandPool>(..., *device) };
  std::unique_ptr<Neki::BufferFactory> bufferFactory{ std::make_unique<Neki::BufferFactory>(..., *device, *commandPool) };

  //...Application Logic...//
}
```

<br></br>
## Development Build
While its primary use is as a static library, NekiVK can be built as a standalone executable to run internal tests.
```bash
git clone https://github.com/Kahoneki/NekiVK.git
cd NekiVK
cmake -B build
cmake --build build
```

<br></br>
## Classes
Classes currently available in NekiVK (and their underlying responsibilities) are:
- **`VulkanDevice`:** `VkInstance`, `VkDevice`, and `VkQueue`
- **`VulkanCommandPool`:** `VkCommandPool`, allocation of `VkCommandBuffer` objects from underlying pool
- **`VulkanDescriptorPool`:** `VkDescriptorPool`, allocation of `VkDescriptorSet` objects from underlying pool
- **`VulkanPipeline` (Pure Virtual):** `VkPipelineLayout`, `VkPipeline`, `VkShaderModule`s
- **`VulkanGraphicsPipeline`:** `VulkanPipeline` subclass implementing graphics pipeline
- **`VulkanSwapchain`:** `GLFWwindow`, `VkSurfaceKHR`, `VkSwapchainKHR`, and `VkImage`s and `VkImageView`s (swapchain images and views)
- **`BufferFactory`:** `VkBuffer`s and `VkDeviceMemory`s
- **`ImageFactory`:** `VkImage`s, `VkImageView`s, `VkDeviceMemory`s, and `VkSampler`s
- **`ModelFactory`:** Loads textured models into an easy-to-use `GPUModel` object.
- **`VKDebugAllocator`:** Optional debug allocator with `VkAllocationCallbacks*`-cast operator overload. Tracks allocations and frees, providing an error message if a memory leak is detected
- **`VKLogger`:** Custom logger with support for channel and layer configuration (e.g.: receiving all output from `DEVICE` layer but only error output from `IMAGE_FACTORY` layer)
