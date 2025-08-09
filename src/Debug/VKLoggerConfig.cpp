#include "NekiVK/Debug/VKLoggerConfig.h"

namespace Neki
{


VKLoggerConfig::VKLoggerConfig(bool _enableAll)
{
	if (_enableAll) { defaultChannelBitfield = VK_LOGGER_CHANNEL::INFO | VK_LOGGER_CHANNEL::HEADING | VK_LOGGER_CHANNEL::WARNING | VK_LOGGER_CHANNEL::ERROR | VK_LOGGER_CHANNEL::SUCCESS; }
	else { defaultChannelBitfield = VK_LOGGER_CHANNEL::NONE; }
}



void VKLoggerConfig::SetDefaultChannelBitfield(VK_LOGGER_CHANNEL _channels)
{
	defaultChannelBitfield = _channels;
}



void VKLoggerConfig::SetLayerChannelBitfield(VK_LOGGER_LAYER _layer, VK_LOGGER_CHANNEL _channels)
{
	layerChannelBitfield[_layer] = _channels;
}



VK_LOGGER_CHANNEL VKLoggerConfig::GetChannelBitfieldForLayer(VK_LOGGER_LAYER _layer) const
{
	//If a specific channel bitfield has been set for this layer, return it. Otherwise, just return the default
	std::unordered_map<VK_LOGGER_LAYER, VK_LOGGER_CHANNEL>::const_iterator it{ layerChannelBitfield.find(_layer) };
	if (it != layerChannelBitfield.end())
	{
		return it->second;
	}
	return defaultChannelBitfield;
}



}