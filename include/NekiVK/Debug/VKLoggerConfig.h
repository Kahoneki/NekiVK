#ifndef VKLOGGERCONFIG_H
#define VKLOGGERCONFIG_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include "NekiVK/Utils/Templates/enum_enable_bitmask_operators.h"

namespace Neki
{
	//Defines the severity of a log message
	enum class VK_LOGGER_CHANNEL : std::uint32_t
	{
		NONE	=	0,
		HEADING =	1 << 0,
		INFO	=	1 << 1,
		WARNING =	1 << 2,
		ERROR	=	1 << 3,
		SUCCESS =	1 << 4,
	};

}

//Enable bitmask operators for the VK_LOGGER_CHANNEL type
template<>
struct enable_bitmask_operators<Neki::VK_LOGGER_CHANNEL> : std::true_type{};


namespace Neki
{

	enum class VK_LOGGER_LAYER : std::uint32_t
	{
		DEVICE,
		COMMAND_POOL,
		SWAPCHAIN,
		RENDER_MANAGER,
		DESCRIPTOR_POOL,
		PIPELINE,
		BUFFER_FACTORY,
		IMAGE_FACTORY,
		MODEL_FACTORY,
		APPLICATION,
	};


	//Config class to be passed to the construction of a VKApp by an application (or to a VKLogger internally by VKApp)
	class VKLoggerConfig
	{
	public:
		explicit VKLoggerConfig(bool _enableAll=false);
		~VKLoggerConfig() = default;

		//Set the default logging channels for all layers that have not been explicitly set
		void SetDefaultChannelBitfield(VK_LOGGER_CHANNEL _channels);

		//Set the logging channels for a specific layer
		void SetLayerChannelBitfield(VK_LOGGER_LAYER _layer, VK_LOGGER_CHANNEL _channels);

		//Get the logging channels for a specific layer
		VK_LOGGER_CHANNEL GetChannelBitfieldForLayer(VK_LOGGER_LAYER _layer) const;

	private:
		VK_LOGGER_CHANNEL defaultChannelBitfield;
		std::unordered_map<VK_LOGGER_LAYER, VK_LOGGER_CHANNEL> layerChannelBitfield;
	};

}

#endif