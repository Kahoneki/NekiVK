#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <string>

struct ImageMetadata
{
	int width;
	int height;
	int channels;
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
	static ImageData Load(const std::string& _filepath);
	static void Free(void* _pixels);
};


#endif