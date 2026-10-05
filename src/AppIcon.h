#pragma once

#include <string>
struct SDL_Window;

// Empty path selects the embedded Judas artwork. Invalid custom images leave
// the window's existing fallback intact. Image pixels are never GL resources.
bool ApplyAppIcon(SDL_Window* window, const std::string& path, std::string& error);
bool ValidateAppIcon(const std::string& path, std::string& error);
bool WriteDefaultAppIcon(const std::string& path, std::string& error);
