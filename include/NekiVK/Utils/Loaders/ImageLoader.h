#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>

struct ImageMetadata
{
	int width;
	int height;
	int channels;
	VkFormat vkFormat;
};

struct ImageData
{
	unsigned char* pixels;
	ImageMetadata metadata;
};


//Static utility class for loading and freeing image data using stb_image
class ImageLoader
{
public:
	static ImageData Load(const std::string& _filepath, bool _flipImage);
	static void Free(void* _pixels);


private:
	//Return from cache if image has already been loaded
	static std::unordered_map<std::string, ImageData> imageCache;
};


#endif