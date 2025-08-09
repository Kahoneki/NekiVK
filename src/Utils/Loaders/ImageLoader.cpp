#include "NekiVK/Utils/Loaders/ImageLoader.h"
#include <stb_image.h>
#include <stdexcept>

ImageData ImageLoader::Load(const std::string& filepath)
{
	stbi_set_flip_vertically_on_load(true);
	
	ImageData imageData{};
	imageData.pixels = stbi_load(filepath.c_str(), &imageData.metadata.width, &imageData.metadata.height, &imageData.metadata.channels, STBI_rgb_alpha);
	if (!imageData.pixels) { throw std::runtime_error("Failed to load texture image: " + filepath); }
	imageData.metadata.channels = 4;
	return imageData;
}



void ImageLoader::Free(void* pixels)
{
	stbi_image_free(pixels);
}