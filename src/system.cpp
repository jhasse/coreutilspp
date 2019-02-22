#include "system.hpp"

#ifdef _WIN32
#include <cstdio>
#include <io.h>

bool mayPrompt() {
	return _isatty(_fileno(stdin));
}
#else
#include <unistd.h>

bool mayPrompt() {
	return isatty(STDIN_FILENO);
}
#endif
