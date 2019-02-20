#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace po = boost::program_options;

std::string binaryName;
po::variables_map vm;
int exitcode = 0;

void remove(const std::string& filename, fs::path path) {
	const auto error = [&filename](const std::string& msg) {
		std::cerr << binaryName << ": cannot remove '" << filename << "': " << msg << "\n";
		exitcode = 1;
	};
	if (fs::is_directory(path)) {
		if (vm.count("recursive") > 0) {
			for (auto& child : fs::directory_iterator(path)) {
				remove(fs::relative(child.path()).string(), child.path());
			}
		} else {
			error("Is a directory");
			return;
		}
	}
	if ((fs::status(path).permissions() & fs::perms::owner_write) == fs::perms::none) {
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

	po::options_description desc("Options");
	desc.add_options()("recursive,r", "remove directories and their contents recursively")(
	    "help", "display this help and exit")("version", "output version information and exit");

	po::options_description hidden("Hidden options");
	hidden.add_options()("files", po::value(&files));

	po::options_description options;
	options.add(desc).add(hidden);

	po::positional_options_description positional_desc;
	positional_desc.add("files", -1);

	try {
		po::store(
		    po::command_line_parser(argc, argv).options(options).positional(positional_desc).run(),
		    vm);
	} catch (boost::program_options::unknown_option& e) {
		std::cerr << binaryName << ": " << e.what() << "\nTry '" << binaryName
		          << " --help' for more information.\n";
		return 1;
	}
	po::notify(vm);

	if (vm.count("help") > 0) {
		std::cout << "Usage: " << binaryName
		          << " [OPTION]... [FILE]...\nRemove (unlink) the FILE(s).\n\n"
		          << desc << "\n";
		return 0;
	}
	if (vm.count("version") > 0) {
		std::cout << binaryName
		          << " 0.1\nCopyright © 2018 Jan Niklas Hasse\nLicense GPLv3+: GNU GPL version 3 "
		             "or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you "
		             "are free to change and redistribute it.\nThere is NO WARRANTY, to the extent "
		             "permitted by law.\n";
		return 0;
	}
	for (const std::string& filename : files) {
		try {
			if (vm.count("recursive") > 0 and vm.count("force") > 0) {
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
