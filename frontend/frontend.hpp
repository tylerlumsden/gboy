#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>

#include "event.hpp"
#include "data_types.hpp"
#include "display.hpp"

struct Window {
    std::string name;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* buffer;

    unsigned int width;
    unsigned int height;
};

struct Frontend {
    std::vector<Window> windows;

    Window create_window(unsigned int width, unsigned int height, std::string name);

    Frontend();
    ~Frontend();

    Frontend(const Frontend&) = delete;
    Frontend& operator=(const Frontend&) = delete;
};


bool render_buffer(Window ctx, Buffer buf);

std::vector<Event_Type> poll_events(Frontend& ctx);