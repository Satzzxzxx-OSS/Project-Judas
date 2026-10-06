#pragma once
#include "Window.h"
#include <SDL2/SDL.h>
#include <string>

// Deterministic application fixtures enter the real M35 physical-input path
// after BeginFrame. Synthetic desktop events are intentionally isolated too.
inline void QueueTestKey(Window& window, SDL_Scancode code, bool down) {
    window.QueueTestPhysical(std::string("key:")+SDL_GetKeyName(SDL_GetKeyFromScancode(code)),down?1.f:0.f);
}
