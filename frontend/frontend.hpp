#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>

#include "event.hpp"

struct Frontend {
    SDL_Window* window;

    Frontend(unsigned int width, unsigned int height);
    ~Frontend();

    Frontend(const Frontend&) = delete;
    Frontend& operator=(const Frontend&) = delete;
};

std::vector<Event_Type> poll_events(Frontend& ctx);