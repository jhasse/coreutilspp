#include <filesystem>
#include <iostream>
#include <lyra/lyra.hpp>
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

struct Entry {
	std::string name;
	bool directory;
};

int main(int argc, char** argv) {
	std::string binaryName = fs::path(argv[0]).filename().string();
	std::string path = ".";
	bool help = false;
	bool version = false;
	auto cli =
	    lyra::cli_parser() |
	    lyra::opt(help)["-h"]["--help"]("display this help and exit") |
	    lyra::opt(version)["--version"]("output version information and exit") |
	    lyra::arg(path, "");

	const auto result = cli.parse(lyra::args(argc, argv));
	if (!result) {
		std::cerr << binaryName << ": " << result.errorMessage() << "\nTry '" << binaryName
		          << " --help' for more information.\n";
		return 1;
	}
	if (help) {
		cli.parse(lyra::args{ "" }); // empty exeName so that Lyra doesn't print usage information
		std::cout << "Usage: " << binaryName
		          << " [OPTION]... [PATH]\nList information about the PATH (the current directory by default).\n\n"
		          << cli;
		return 0;
	}
	if (version) {
		std::cout << binaryName
		          << " 0.1\nCopyright © 2022 Jan Niklas Hasse\nLicense GPLv3+: GNU GPL version 3 "
		             "or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you "
		             "are free to change and redistribute it.\nThere is NO WARRANTY, to the extent "
		             "permitted by law.\n";
		return 0;
	}

	bool supportsColor = true;
#ifdef _WIN32
	const auto console = GetStdHandle(STD_OUTPUT_HANDLE);
	// Try enabling ANSI escape sequence support on Windows 10 terminals.
	if (supportsColor) {
		DWORD mode;
		if (GetConsoleMode(console, &mode)) {
			if (!SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
				supportsColor = false;
			}
		}
	}
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(console, &csbi);
	const int columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
	const int columns = 80; // TODO
#endif

	int maxWidth = 0;
	std::vector<Entry> files;
	for (const auto& file : fs::directory_iterator(path)) {
		files.emplace_back(Entry{ file.path().filename().u8string(), file.is_directory() });
		if (files.back().name.size() /* TODO: UTF-8 */ > maxWidth) {
			maxWidth = files.back().name.size();
		}
	}

	maxWidth += 2; // two spaces between columns

	int columnsPrinted = 0;
	bool resetColor = false;
	for (const auto& file : files) {
		if (file.directory && supportsColor) {
			std::cout << "\x1b[1;34m";
			resetColor = true;
		} else if (resetColor) {
			std::cout << "\x1b[0m";
			resetColor = false;
		}
		if (columnsPrinted > 0 && columnsPrinted + maxWidth > columns) {
			std::cout << std::endl;
			columnsPrinted = 0;
		}
		std::cout << file.name;
		columnsPrinted += maxWidth;
		if (columnsPrinted > columns) {
			std::cout << std::endl;
			columnsPrinted = 0;
		} else {
			std::cout << std::string(maxWidth - file.name.size(), ' ');
		}
	}
	if (resetColor) {
		std::cout << "\x1b[0m";
	}
	if (columnsPrinted > 0) {
		std::cout << std::endl;
	}
}
