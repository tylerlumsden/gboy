#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <array>
#include <stdexcept>

#include "frontend.hpp"
#include "event.hpp"

Frontend::Frontend(unsigned int width, unsigned int height) {
    SDL_Init(SDL_INIT_VIDEO);

    if(!SDL_CreateWindowAndRenderer(
        "Window", 
        width, height, 
        0, 
        &this->window, 
        &this->renderer)) {
            throw std::runtime_error("Unable to allocate SDL window and renderer");
        }
    
    this->buffer = SDL_CreateTexture(
        this->renderer, 
        SDL_PIXELFORMAT_ARGB8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        width,
        height
    );

    if(!buffer) {
        throw std::runtime_error("Unable to allocate SDL texture");
    }
}

Frontend::~Frontend() {
    SDL_DestroyWindow(this->window);
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyTexture(this->buffer);
    SDL_Quit();
}

bool render_buffer(Frontend& ctx, const std::array<uint32_t, 160 * 144>& frame_buffer) {
    if(!SDL_UpdateTexture(ctx.buffer, NULL, frame_buffer.data(), 160 * sizeof(uint32_t))) {
        return false;
    };
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
