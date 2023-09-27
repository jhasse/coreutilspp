#include <filesystem>
#include <iostream>
#include <lyra/lyra.hpp>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
	std::string binaryName = fs::path(argv[0]).filename().string();
	std::vector<std::string> command;
	bool help = false;
	bool version = false;
	std::string changeDir;
	auto cli = lyra::cli_parser() |
	           lyra::opt(changeDir, "DIR")["-C"]["--chdir"]("change working directory to DIR") |
	           lyra::opt(help)["-h"]["--help"]("display this help and exit") |
	           lyra::opt(version)["--version"]("output version information and exit") |
	           lyra::arg(command, "");

	const auto result = cli.parse(lyra::args(argc, argv));
	if (!result) {
		std::cerr << binaryName << ": " << result.errorMessage() << "\nTry '" << binaryName
		          << " --help' for more information.\n";
		return 1;
	}
	if (help) {
		cli.parse(lyra::args{ "" }); // empty exeName so that Lyra doesn't print usage information
		std::cout << "Usage: " << binaryName << " [OPTION]... [COMMAND]\nRun COMMAND.\n\n" << cli;
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
	if (!changeDir.empty()) {
		try {
			fs::current_path(changeDir);
		} catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
			return EXIT_FAILURE;
		}
	}
	std::ostringstream tmp;
	for (const auto& arg : command) {
		tmp << arg << ' ';
	}
	return system(tmp.str().c_str());
}
