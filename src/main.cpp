#include "system.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <lyra/lyra.hpp>
#include <mutex>
#include <string>
#include <thread>
#include <ThreadPool.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

void removeRecursive(fs::path path);

std::string binaryName;
bool recursive = false;
bool force = false;
int exitcode = 0;

bool askYes() {
	std::string input;
	std::getline(std::cin, input);
	std::transform(input.begin(), input.end(), input.begin(),
	               [](unsigned char c) { return std::tolower(c); });
	return input == "y" || input == "yes";
}

void remove(const std::string& filename, fs::path path) {
	const auto error = [&filename](const std::string& msg) {
		std::cerr << binaryName << ": cannot remove '" << filename << "': " << msg << "\n";
		exitcode = 1;
	};
	if (!fs::is_symlink(path) && fs::is_directory(path)) {
		if (recursive) {
			if (force) {
				removeRecursive(path);
			} else {
				for (auto& child : fs::directory_iterator(path)) {
					remove(fs::relative(child.path()).string(), child.path());
				}
			}
		} else {
			error("Is a directory");
			return;
		}
	}
	if (!force && mayPrompt() &&
	    (fs::status(path).permissions() & fs::perms::owner_write) == fs::perms::none) {
		std::cout << binaryName << ": remove write-protected file '" << filename << "'? ";
		if (!askYes()) {
			return;
		}
	}
	if (!fs::remove(path) && !force) {
		error("No such file or directory");
	}
}

void removeRecursive(fs::path path) {
	std::atomic_uint64_t deletedFiles = 0;
	std::atomic_uint64_t filesToDelete = 0;
	bool hasPrinted = false;

	std::string error;
	std::mutex errorMutex;

	std::atomic_bool requestStop = false;
	std::condition_variable requestStopCV;
	std::mutex requestStopMutex;

	std::thread uiThread([&]() {
		while (!requestStop || deletedFiles < filesToDelete) {
			std::unique_lock cvLock(requestStopMutex);
			requestStopCV.wait_for(cvLock, std::chrono::milliseconds(10));
			if (filesToDelete > 0) {
				hasPrinted = true;
				std::cout << "\r"
				          << "(" << deletedFiles << " / " << filesToDelete << ") ... ";
				if (!requestStop) {
					std::cout << "~";
				}
				std::cout << (deletedFiles * 100 / filesToDelete) << "% " << std::flush;
			}
		}
	});

	{
		ThreadPool pool(std::thread::hardware_concurrency());

		for (auto& it : fs::recursive_directory_iterator(path)) {
			if (requestStop) {
				break;
			}
			++filesToDelete;
			pool.enqueue([it, &deletedFiles, &error, &errorMutex, &requestStop]() {
				try {
#ifdef _WIN32
					const auto path = it.path().wstring();
					DWORD attributes = GetFileAttributes(path.c_str());
					if (attributes == INVALID_FILE_ATTRIBUTES &&
					    GetLastError() == ERROR_FILE_NOT_FOUND) {
						throw std::exception();
					}
					if (attributes & FILE_ATTRIBUTE_READONLY) {
						// On non-Windows systems remove will happily delete read-only files. On
						// Windows rm++ should behave the same. See
						// https://github.com/ninja-build/ninja/issues/1886
						SetFileAttributes(path.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
					}
					if (!DeleteFile(path.c_str()) && !fs::is_directory(it)) {
						throw std::exception();
					}
#else
					// directories will be removed at the end using remove_all
					if (!fs::is_directory(it) && !fs::remove(it)) {
						throw std::runtime_error("files doesn't exist");
					}
#endif
				} catch (std::exception&) {
					std::unique_lock lk(errorMutex);
					error = it.path().string();
					requestStop = true;
				}
				++deletedFiles;
			});
		}

		requestStop = true;
		requestStopCV.notify_all();
	}

	uiThread.join();
	if (error.empty()) {
		if (hasPrinted) {
			std::cout << "\33[2K\r" << std::flush;
		}
		fs::remove_all(path); // remove empty directories
	} else {
		std::cerr << "\nCouldn't delete " << error << std::endl;
		exitcode = 1;
	}
}

// See https://stackoverflow.com/a/15549954/647898
bool pathContainsFile(fs::path dir, fs::path file) {
	// If dir ends with "/" and isn't the root directory, then the final component returned by
	// iterators will include "." and will interfere with the std::equal check below, so we strip it
	// before proceeding.
	if (dir.filename() == ".") {
		dir.remove_filename();
	}
	// We're also not interested in the file's name.
	assert(file.has_filename());
	file.remove_filename();

	// If dir has more components than file, then file can't possibly reside in dir.
	auto dir_len = std::distance(dir.begin(), dir.end());
	auto file_len = std::distance(file.begin(), file.end());
	if (dir_len > file_len) {
		return false;
	}
	// This stops checking when it reaches dir.end(), so it's OK if file has more directory
	// components afterward. They won't be checked.
	return std::equal(dir.begin(), dir.end(), file.begin());
}

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
	    lyra::opt(force)["-f"]["--force"]("ignore nonexistent files and arguments, never prompt") |
	    lyra::opt(recursive)["-r"]["-R"]["--recursive"](
	        "remove directories and their contents recursively") |
	    lyra::opt(help)["-h"]["--help"]("display this help and exit") |
	    lyra::opt(version)["--version"]("output version information and exit") |
	    lyra::arg(files, "");

	const auto result = cli.parse(lyra::args(argc, argv));
	if (!result) {
		std::cerr << binaryName << ": " << result.errorMessage() << "\nTry '" << binaryName
		          << " --help' for more information.\n";
		return 1;
	}

	if (help) {
		cli.parse(lyra::args{ "" }); // empty exeName so that Lyra doesn't print usage information
		std::cout << "Usage: " << binaryName
		          << " [OPTION]... [FILE]...\nRemove (unlink) the FILE(s).\n\n"
		          << cli;
		return 0;
	}
	if (version) {
		std::cout << binaryName
		          << " 0.2\nCopyright © 2019 Jan Niklas Hasse\nLicense GPLv3+: GNU GPL version 3 "
		             "or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you "
		             "are free to change and redistribute it.\nThere is NO WARRANTY, to the extent "
		             "permitted by law.\n";
		return 0;
	}
	const auto cwd = fs::current_path();
	fs::path home;
#ifdef _WIN32
	if (const auto envHome = getEnv("USERPROFILE")) {
#else
	if (const auto envHome = getEnv("HOME")) {
#endif
		home = fs::canonical(*envHome);
#ifdef _WIN32
	} else if (const auto envUser = getEnv("USERNAME")) {
		home = fs::path("C:/Users") / fs::path(*envUser);
#else
	} else if (const auto envUser = getEnv("USER")) {
		home = fs::path("/home") / fs::path(*envUser);
#endif
	}

	// Warn when we're getting passed 2 folders and 2 files or more. This helps to prevent an
	// accidental `rm++ -rf *` in big directories.
	if (mayPrompt() && files.size() > 3) {
		size_t numberOfFolders = 0;
		for (const auto& filename : files) {
			if (fs::is_directory(filename)) {
				++numberOfFolders;
			}
		}
		const size_t numberOfFiles = files.size() - numberOfFolders;
		if (numberOfFiles > 1 && numberOfFolders > 1) {
			std::cout << "Delete " << numberOfFolders << " folders and " << numberOfFiles
			          << " files? ";
			if (!askYes()) {
				return 1;
			}
		}
	}

	for (const std::string& filename : files) {
		try {
			fs::path path(filename);
			if (recursive) {
				const auto canonicalPath = fs::canonical(path);
				if (canonicalPath == home) {
					std::cout << "You're trying to delete your home directory. ";
					if (mayPrompt()) {
						std::cout << "Continue? ";
						if (!askYes()) {
							continue;
						}
					} else {
						std::cout << "Skipping.\n";
						exitcode = 1;
						continue;
					}
				} else if (pathContainsFile(canonicalPath, home)) {
					std::cout
					    << "You're trying to delete a path which is above your home directory. ";
					if (mayPrompt()) {
						std::cout << "Continue? ";
						if (!askYes()) {
							continue;
						}
					} else {
						std::cout << "Skipping.\n";
						exitcode = 1;
						continue;
					}
				} else if (mayPrompt()) {
					if (canonicalPath == cwd) {
						std::cout
						    << "You're trying to delete your current working directory. Continue? ";
						if (!askYes()) {
							continue;
						}
					} else if (pathContainsFile(canonicalPath, cwd)) {
						std::cout << "You're trying to delete a path which is above your current "
						             "working directory. Continue? ";
						if (!askYes()) {
							continue;
						}
					}
				}
			}
			remove(filename, path);
		} catch (fs::filesystem_error& err) {
			const auto code = err.code();
			// --force should ignore non-existant files
			if (code.value() != 2 /* No such file or directory */ || !force) {
				std::cerr << binaryName << ": cannot remove '" << err.path1().string()
				          << "': " << code.message() << std::endl;
				exitcode = 1;
			}
		}
	}
	return exitcode;
}
