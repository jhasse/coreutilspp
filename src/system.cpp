#include "system.hpp"

#ifdef _WIN32
#include <cstdio>
#include <io.h>

bool mayPrompt() {
	return _isatty(_fileno(stdin));
}

std::optional<std::string> getEnv(const char* const variableName) {
	char* envTmp = nullptr;
	size_t envTmpSize;
	_dupenv_s(&envTmp, &envTmpSize, "USERPROFILE");
	std::string envHome;
	if (envTmp) {
		std::string tmp(envTmp, envTmpSize);
		free(envTmp);
		return tmp;
	}
	return {};
}

#else
#include <unistd.h>

bool mayPrompt() {
	return isatty(STDIN_FILENO);
}

std::optional<std::string> getEnv(const char* const variableName) {
	char* envTmp = std::getenv(variableName);
	if (envTmp) {
		return std::string(envTmp);
	}
	return {};
}
#endif
