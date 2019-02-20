#include <boost/algorithm/string.hpp>
#include <clara.hpp>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::string binaryName;
bool recursive = false;
bool force = false;
int exitcode = 0;

void remove(const std::string& filename, fs::path path) {
	const auto error = [&filename](const std::string& msg) {
		std::cerr << binaryName << ": cannot remove '" << filename << "': " << msg << "\n";
		exitcode = 1;
	};
	if (fs::is_directory(path)) {
		if (recursive) {
			for (auto& child : fs::directory_iterator(path)) {
				remove(fs::relative(child.path()).string(), child.path());
			}
		} else {
			error("Is a directory");
			return;
		}
	}
	if (!force and (fs::status(path).permissions() & fs::perms::owner_write) == fs::perms::none) {
		std::cout << binaryName << ": remove write-protected file '" << filename << "'? ";
		std::string input;
		std::getline(std::cin, input);
		boost::algorithm::to_lower(input);
		if (input != "y" && input != "yes") {
			return;
		}
	}
	if (!fs::remove(path)) {
		error("No such file or directory");
	}
}

int main(int argc, char** argv) {
	binaryName = fs::path(argv[0]).filename().string();

	std::vector<std::string> files;
	bool help = false;
	bool version = false;
	auto cli =
	    clara::Parser() |
	    clara::Opt(force)["-f"]["--force"]("ignore nonexistent files and arguments, never prompt") |
	    clara::Opt(recursive)["-r"]["-R"]["--recursive"](
	        "remove directories and their contents recursively") |
	    clara::Opt(help)["-h"]["--help"]("display this help and exit") |
	    clara::Opt(version)["--version"]("output version information and exit") |
	    clara::Arg(files, "");

	const auto result = cli.parse(clara::Args(argc, argv));
	if (!result) {
		std::cerr << binaryName << ": " << result.errorMessage() << "\nTry '" << binaryName
		          << " --help' for more information.\n";
		return 1;
	}

	if (help) {
		cli.parse(clara::Args{""}); // empty exeName so that Clara doesn't print usage information
		std::cout << "Usage: " << binaryName
		          << " [OPTION]... [FILE]...\nRemove (unlink) the FILE(s).\n\n"
		          << cli << "\n";
		return 0;
	}
	if (version) {
		std::cout << binaryName
		          << " 0.1\nCopyright © 2018 Jan Niklas Hasse\nLicense GPLv3+: GNU GPL version 3 "
		             "or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you "
		             "are free to change and redistribute it.\nThere is NO WARRANTY, to the extent "
		             "permitted by law.\n";
		return 0;
	}
	for (const std::string& filename : files) {
		try {
			if (recursive and force) {
				fs::remove_all(fs::path(filename));
			} else {
				remove(filename, fs::path(filename));
			}
		} catch (fs::filesystem_error& err) {
			std::cerr << binaryName << ": cannot remove '" << err.path1().string()
			          << "': " << err.code().message() << std::endl;
		}
	}
	return exitcode;
}
