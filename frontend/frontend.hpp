#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

struct Frontend {
    SDL_Window* window;

    Frontend(unsigned int width, unsigned int height);
    ~Frontend();
};
