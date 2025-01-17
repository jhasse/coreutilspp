#include <filesystem>
#include <fstream>
#include <iostream>
#include <lyra/lyra.hpp>
#include <vector>

namespace fs = std::filesystem;

std::string binaryName;
int exitcode = 0;

/// use this file's times instead of current time
std::string reference;

bool noCreate = false;

int main(int argc, char** argv) {
	if (argc == 1) {
		std::cerr << argv[0] << ": missing operand\nTry '" << argv[0]
		          << " --help' for more information.\n";
		return 1;
	}
	binaryName = fs::path(argv[0]).filename().string();

	std::vector<std::string> files;
	bool help = false;
	bool version = false;
	auto cli =
	    lyra::cli_parser() |
	    lyra::opt(noCreate)["-c"]["--no-create"]("do not create any files") |
	    lyra::opt(reference, "FILE")["-r"]["--reference"]("use this file's times instead of current time") |
	    lyra::opt(help)["-h"]["--help"]("display this help and exit") |
	    lyra::opt(version)["--version"]("output version information and exit") |
	    lyra::arg(files, "");

	const auto result = cli.parse(lyra::args(argc, argv));
	if (!result) {
		std::cerr << binaryName << ": " << result.message() << "\nTry '" << binaryName
		          << " --help' for more information.\n";
		return 1;
	}

	if (help) {
		cli.parse(lyra::args{ "" }); // empty exeName so that Lyra doesn't print usage information
		std::cout << "Usage: " << binaryName
		          << " [OPTION]... [FILE]...\nUpdate the access and modification times of each "
		             "FILE to the current time.\n\n"
		          << cli;
		return 0;
	}
	if (version) {
		std::cout << binaryName
		          << " 0.1\nCopyright © 2019 Jan Niklas Hasse\nLicense GPLv3+: GNU GPL version 3 "
		             "or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you "
		             "are free to change and redistribute it.\nThere is NO WARRANTY, to the extent "
		             "permitted by law.\n";
		return 0;
	}

	for (const std::string& filename : files) {
		try {
			fs::path path(filename);
			fs::file_time_type timestamp = fs::file_time_type::clock::now();
			if (!reference.empty()) {
				timestamp = fs::last_write_time(fs::path(reference));
			}
			if (fs::exists(path)) {
				fs::last_write_time(path, timestamp);
			} else {
				if (!noCreate) {
					std::ofstream tmp(path);
				}
			}
		} catch (fs::filesystem_error& err) {
			const auto code = err.code();
			if (code.value() != 2 /* No such file or directory */) {
				std::cerr << binaryName << ": cannot remove '" << err.path1().string()
				          << "': " << code.message() << std::endl;
				exitcode = 1;
			}
		}
	}
	return exitcode;
}
