#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <array>
#include <stdexcept>

#include "frontend.hpp"
#include "event.hpp"

Frontend::Frontend() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
}

Window Frontend::create_window(unsigned int width, unsigned int height, std::string name) {

    Window new_window;

    new_window.name = name;

    if(!SDL_CreateWindowAndRenderer(
        name.c_str(), 
        width, height, 
        0, 
        &new_window.window, 
        &new_window.renderer)) {
            throw std::runtime_error("Unable to allocate SDL window and renderer");
    }
    
    new_window.buffer = SDL_CreateTexture(
        new_window.renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        GB_Width,
        GB_Height
    );

    if(!new_window.buffer) {
        throw std::runtime_error("Unable to allocate SDL texture");
    }

    return new_window;
}

Frontend::~Frontend() {
    for(Window window : windows) {
        SDL_DestroyWindow(window.window);
        SDL_DestroyRenderer(window.renderer);
        SDL_DestroyTexture(window.buffer);
    }

    SDL_Quit();
}

bool render_buffer(Window ctx, const FrameBuffer& frame_buffer) {
    if(!SDL_UpdateTexture(ctx.buffer, NULL, frame_buffer.data(), GB_Width * sizeof(Quad_Byte))) {
        return false;
    }
    if(!SDL_RenderClear(ctx.renderer)) {
        return false;
    }
    if(!SDL_RenderTexture(ctx.renderer, ctx.buffer, NULL, NULL)) {
        return false;
    }
    if(!SDL_RenderPresent(ctx.renderer)) {
        return false;
    }
    return true;
}

std::vector<Event_Type> poll_events(Frontend& ctx) {
    std::vector<Event_Type> events;

    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        switch(e.type) {
        case SDL_EVENT_QUIT:
            events.push_back(Event_Type::QUIT);
        }
    }

    return events;
}
