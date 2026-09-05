#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "frontend.hpp"
#include "event.hpp"

Frontend::Frontend(unsigned int width, unsigned int height) {
    SDL_Init(SDL_INIT_VIDEO);
    this->window = SDL_CreateWindow("Window", width, height, 0);
}

Frontend::~Frontend() {
    SDL_DestroyWindow(this->window);
    SDL_Quit();
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
