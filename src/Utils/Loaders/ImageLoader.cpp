#include "NekiVK/Utils/Loaders/ImageLoader.h"
#include <stb_image.h>
#include <stdexcept>

ImageData ImageLoader::Load(const std::string& _filepath)
{
	stbi_set_flip_vertically_on_load(true);
	
	ImageData imageData{};
	imageData.pixels = stbi_load(_filepath.c_str(), &imageData.metadata.width, &imageData.metadata.height, &imageData.metadata.channels, STBI_rgb_alpha);
	if (!imageData.pixels) { throw std::runtime_error("Failed to load texture image: " + _filepath); }
	imageData.metadata.channels = 4;
	return imageData;
}



void ImageLoader::Free(void* _pixels)
{
	stbi_image_free(_pixels);
}