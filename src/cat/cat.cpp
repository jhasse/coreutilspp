#include <filesystem>
#include <fstream>
#include <iostream>
#include <lyra/lyra.hpp>

namespace fs = std::filesystem;

struct Entry {
	std::string name;
	bool directory;
};

int main(int argc, char** argv) {
	std::string binaryName = fs::path(argv[0]).filename().string();
	std::string file = "";
	bool help = false;
	bool version = false;
	auto cli = lyra::cli_parser() | lyra::opt(help)["-h"]["--help"]("display this help and exit") |
	           lyra::opt(version)["--version"]("output version information and exit") |
	           lyra::arg(file, "");

	const auto result = cli.parse(lyra::args(argc, argv));
	if (!result) {
		std::cerr << binaryName << ": " << result.message() << "\nTry '" << binaryName
		          << " --help' for more information.\n";
		return 1;
	}
	if (help) {
		cli.parse(lyra::args{ "" }); // empty exeName so that Lyra doesn't print usage information
		std::cout << "Usage: " << binaryName
		          << " [OPTION]... [FILE]\nConcatenate FILE(s), or standard input, to standard output.\n\n"
		          << cli;
		return 0;
	}
	if (version) {
		std::cout << binaryName
		          << " 0.3\nCopyright © 2022 Jan Niklas Hasse\nLicense GPLv3+: GNU GPL version 3 "
		             "or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you "
		             "are free to change and redistribute it.\nThere is NO WARRANTY, to the extent "
		             "permitted by law.\n";
		return 0;
	}

	if (file.empty()) {
		std::cout << std::cin.rdbuf();
		return EXIT_SUCCESS;
	}
	std::ifstream fin(file, std::ios::binary);
	if (!fin) {
		std::cerr << binaryName << ": " << file << ": No such file or directory" << std::endl;
		return EXIT_FAILURE;
	}
	std::cout << fin.rdbuf();
}
