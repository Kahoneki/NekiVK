#ifndef VKLOGGER_H
#define VKLOGGER_H

#include "VKLoggerConfig.h"
#include <string>


namespace Neki
{


	enum class VK_LOGGER_WIDTH : std::uint32_t
	{
		DEFAULT = 0,
		SUCCESS_FAILURE = 80,

		//For internal use
		CHANNEL = 10,
		LAYER = 20,
	};


	class VKLogger final
	{
	public:
		explicit VKLogger(const VKLoggerConfig& _config);
		~VKLogger() = default;

		//Logs _message from _layer to the _channel channel (if the channel is enabled for the specified layer)
		//Optionally include a text width
		//Newline characters are not included by this function, they should be contained within _message
		//Optionally set _formatted flag to false to print the raw text
		void Log(VK_LOGGER_CHANNEL _channel, VK_LOGGER_LAYER _layer, const std::string& _message, VK_LOGGER_WIDTH _width=VK_LOGGER_WIDTH::DEFAULT, bool _formatted=true) const;


		//ANSI colour codes
		static constexpr const char* COLOUR_RED   { "\033[31m" };
		static constexpr const char* COLOUR_GREEN { "\033[32m" };
		static constexpr const char* COLOUR_YELLOW{ "\033[33m" };
		static constexpr const char* COLOUR_CYAN  { "\033[36m" };
		static constexpr const char* COLOUR_WHITE { "\033[97m" }; //37 (regular white) is quite dark, use 97 (high intensity white)
		static constexpr const char* COLOUR_RESET { "\033[0m"  };
		
		
	private:
		//For older Windows systems
		//Attempt to enable ANSI support for access to ANSI colour codes
		//Returns success status
		static bool EnableAnsiSupport();

		static std::string LayerToString(VK_LOGGER_LAYER _layer);

		
		const VKLoggerConfig config;
	};
}



#endif