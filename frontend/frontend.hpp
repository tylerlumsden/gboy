#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>

#include "event.hpp"



struct Frontend {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* buffer;

    Frontend(unsigned int width, unsigned int height);
    ~Frontend();

    Frontend(const Frontend&) = delete;
    Frontend& operator=(const Frontend&) = delete;
};


bool render_buffer(Frontend& ctx, const std::array<uint32_t, 160 * 144>& frame_buffer);

std::vector<Event_Type> poll_events(Frontend& ctx);