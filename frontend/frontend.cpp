#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "frontend.hpp"

Frontend::Frontend(unsigned int width, unsigned int height) {
    SDL_Init(SDL_INIT_VIDEO);
    this->window = SDL_CreateWindow("Window", width, height, 0);
}

Frontend::~Frontend() {
    SDL_DestroyWindow(this->window);
    SDL_Quit();
}