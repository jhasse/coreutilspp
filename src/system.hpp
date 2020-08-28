#include <optional>
#include <string>

/// Whether the user is able to provide input on stdin
bool mayPrompt();

std::optional<std::string> getEnv(const char* variableName);
