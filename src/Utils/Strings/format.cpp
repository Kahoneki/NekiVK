#include "NekiVK/Utils/Strings/format.h"

std::string GetFormattedSizeString(std::size_t numBytes)
{
	if (numBytes / (1024 * 1024 * 1024) != 0) { return std::to_string(numBytes / (1024 * 1024 * 1024)) + "GiB"; }
	else if (numBytes / (1024 * 1024) != 0) { return std::to_string(numBytes / (1024 * 1024)) + "MiB"; }
	else if (numBytes / 1024 != 0) { return std::to_string(numBytes / 1024) + "KiB"; }
	else { return std::to_string(numBytes) + "B"; }
}