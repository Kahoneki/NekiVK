#include "NekiVK/Debug/VKLogger.h"
#include "NekiVK/Debug/VKLoggerConfig.h"
#include <iostream>
#include <iomanip>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Neki
{

	VKLogger::VKLogger(const VKLoggerConfig& _config)
		: config(_config)
	{
		if (!EnableAnsiSupport())
		{
			std::cerr << "ERR::VKLOGGER::VKLOGGER::FAILED_TO_ENABLE_ANSI_SUPPORT::FALLING_BACK_TO_DEFAULT_CHANNEL" << std::endl;
		}
	}



	bool VKLogger::EnableAnsiSupport()
	{
		#if defined(_WIN32)
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut == INVALID_HANDLE_VALUE) { return false; }
		DWORD dwMode = 0;
		if (!GetConsoleMode(hOut, &dwMode)) { return false; }
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (!SetConsoleMode(hOut, dwMode)) { return false; }
		#endif

		return true;
	}



	void VKLogger::Log(VK_LOGGER_CHANNEL _channel, VK_LOGGER_LAYER _layer, const std::string& _message, VK_LOGGER_WIDTH _width, bool _formatted) const
	{

		//Check if channel is enabled for the specified layer
		//If it's not, just early return
		if ((config.GetChannelBitfieldForLayer(_layer) & _channel) == VK_LOGGER_CHANNEL::NONE) { return; }

		std::string channelStr;
		std::string colourCode;

		switch (_channel)
		{
		case VK_LOGGER_CHANNEL::INFO:
			channelStr = "[INFO]";
			colourCode = COLOUR_WHITE;
			break;
		case VK_LOGGER_CHANNEL::HEADING:
			channelStr = "[HEADING]";
			colourCode = COLOUR_CYAN;
			break;
		case VK_LOGGER_CHANNEL::WARNING:
			channelStr = "[WARNING]";
			colourCode = COLOUR_YELLOW;
			break;
		case VK_LOGGER_CHANNEL::ERROR:
			channelStr = "[ERROR]";
			colourCode = COLOUR_RED;
			break;
		case VK_LOGGER_CHANNEL::SUCCESS:
			channelStr = "[SUCCESS]";
			colourCode = COLOUR_GREEN;
			break;
		default:
			std::cerr << std::endl << "ERR::VKLOGGER::LOG::DEFAULT_SEVERITY_CASE_REACHED::CHANNEL_WAS_" << std::to_string(static_cast<std::underlying_type_t<VK_LOGGER_CHANNEL>>(_channel)) << "::FALLING_BACK_TO_DEFAULT_CHANNEL" << std::endl;
			channelStr = "[DEFAULT]";
			colourCode = COLOUR_RESET;
			break;
		}

		//std::cerr for errors, std::cout for everything else
		std::ostream& stream{ (_channel == VK_LOGGER_CHANNEL::ERROR) ? std::cerr : std::cout };
		stream << colourCode;
		if (_formatted)
		{
			stream << std::left << std::setw(static_cast<std::underlying_type_t<VK_LOGGER_WIDTH>>(VK_LOGGER_WIDTH::CHANNEL)) << channelStr <<
					  std::left << std::setw(static_cast<std::underlying_type_t<VK_LOGGER_WIDTH>>(VK_LOGGER_WIDTH::LAYER)) << LayerToString(_layer);	
		}
		if (_width != VK_LOGGER_WIDTH::DEFAULT) { stream << std::left << std::setw(static_cast<std::underlying_type_t<VK_LOGGER_WIDTH>>(_width)); }
		stream << _message << COLOUR_RESET;
	}


	std::string VKLogger::LayerToString(VK_LOGGER_LAYER _layer)
	{
		switch (_layer)
		{
			case VK_LOGGER_LAYER::DEVICE:			return "[DEVICE]";
			case VK_LOGGER_LAYER::COMMAND_POOL:		return "[COMMAND POOL]";
			case VK_LOGGER_LAYER::SWAPCHAIN:		return "[SWAPCHAIN]";
			case VK_LOGGER_LAYER::RENDER_MANAGER:	return "[RENDER MANAGER]";
			case VK_LOGGER_LAYER::DESCRIPTOR_POOL:	return "[DESCRIPTOR POOL]";
			case VK_LOGGER_LAYER::PIPELINE:			return "[PIPELINE]";
			case VK_LOGGER_LAYER::BUFFER_FACTORY:	return "[BUFFER FACTORY]";
			case VK_LOGGER_LAYER::IMAGE_FACTORY:	return "[IMAGE FACTORY]";
			case VK_LOGGER_LAYER::APPLICATION:		return "[APPLICATION]";
			default:								return "[UNDEFINED]";
		}
	}

}