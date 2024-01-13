#pragma once
#pragma warning(push, 0)
#ifndef _CONSOLE_COLOURS_H
#define _CONSOLE_COLOURS_H
#include <string>
// TODO: (Syl) Should this actually be MSC_VER instead?
#ifndef WIN32
namespace Console {
	std::string Reset = "\x1b[0m";
	std::string Bright = "\x1b[1m";
	std::string Dim = "\x1b[2m";
	std::string Underscore = "\x1b[4m";
	std::string Blink = "\x1b[5m";
	std::string Reverse = "\x1b[7m";
	std::string Hidden = "\x1b[8m";
	namespace Foreground {
		std::string Black = "\x1b[30m";
		std::string Red = "\x1b[31m";
		std::string Green = "\x1b[32m";
		std::string Yellow = "\x1b[33m";
		std::string Blue = "\x1b[34m";
		std::string Magenta = "\x1b[35m";
		std::string Cyan = "\x1b[36m";
		std::string White = "\x1b[37m";
	}
	namespace Background {
		std::string Black = "\x1b[40m";
		std::string Red = "\x1b[41m";
		std::string Green = "\x1b[42m";
		std::string Yellow = "\x1b[43m";
		std::string Blue = "\x1b[44m";
		std::string Magenta = "\x1b[45m";
		std::string Cyan = "\x1b[46m";
		std::string White = "\x1b[47m";
	}
	using namespace Foreground;
}
#else
// TODO: https://stackoverflow.com/questions/63913005/how-to-test-if-console-supports-ansi-color-codes
namespace Console {
	std::string Reset {};
	std::string Bright {};
	std::string Dim {};
	std::string Underscore {};
	std::string Blink {};
	std::string Reverse {};
	std::string Hidden {};
	namespace Foreground {
		std::string Black {};
		std::string Red {};
		std::string Green {};
		std::string Yellow {};
		std::string Blue {};
		std::string Magenta {};
		std::string Cyan {};
		std::string White {};
	}
	namespace Background {
		std::string Black {};
		std::string Red {};
		std::string Green {};
		std::string Yellow {};
		std::string Blue {};
		std::string Magenta {};
		std::string Cyan {};
		std::string White {};
	}
	using namespace Foreground;
}
#endif
#endif
