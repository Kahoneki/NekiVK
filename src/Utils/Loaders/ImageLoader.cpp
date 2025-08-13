#include "NekiVK/Utils/Loaders/ImageLoader.h"

#include <algorithm>
#include <iostream>
#include <stb_image.h>
#include <stdexcept>

std::unordered_map<std::string, ImageData> ImageLoader::imageCache;



ImageData ImageLoader::Load(const std::string& _filepath, bool _flipImage)
{
	//Return from cache if it exists
	if (std::unordered_map<std::string, ImageData>::iterator it{ imageCache.find(_filepath) }; it != imageCache.end())
	{
		return it->second;
	}

	//Load image data
	stbi_set_flip_vertically_on_load(_flipImage);
	ImageData imageData{};
	imageData.pixels = stbi_load(_filepath.c_str(), &imageData.metadata.width, &imageData.metadata.height, &imageData.metadata.channels, 4); //Todo: don't force 4 channels (my gpu doesn't support 3 though)
	imageData.metadata.channels = 4;
	if (!imageData.pixels) { throw std::runtime_error("Failed to load texture image: " + _filepath); }

	//Add to cache
	imageCache[_filepath] = imageData;

	return imageData;
}



void ImageLoader::Free(void* _pixels)
{
	//Remove from cache
	for (std::pair<std::string, ImageData> imgCacheEntry : imageCache)
	{
		if (imgCacheEntry.second.pixels == _pixels)
		{
			imageCache.erase(imgCacheEntry.first);
			break;
		}
	}

	stbi_image_free(_pixels);
}