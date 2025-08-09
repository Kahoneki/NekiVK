#include <iostream>
#include <NekiVK/NekiVK.h>

int main()
{
	Neki::VKLoggerConfig config(true);
	Neki::VKLogger logger(config);
	logger.Log(Neki::VK_LOGGER_CHANNEL::SUCCESS, Neki::VK_LOGGER_LAYER::APPLICATION, "NekiVK library built and linked successfully!\n");

	std::cout << "Test complete. Press Enter to exit.\n";
	std::cin.get();

	return 0;
}