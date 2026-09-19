#include <fstream>
#include <iostream>

#include "gameboy.hpp"
#include "frontend.hpp"
#include "event.hpp"
#include "log.hpp"

#include "display.hpp"

int main(int argc, char ** argv) {
    try {
        if(argc < 2) {
            throw std::invalid_argument("Requires path to gameboy rom.");
        }

        Cartridge game = construct_cartridge(std::ifstream(argv[1]));
        
        write_as_hex(game.rom.data, std::ofstream("resources/test.dat"));

        Frontend ctx;
        Window game_window = ctx.create_window(GB_Width, GB_Height, "game");

        #ifndef NDEBUG
        Window tile_window = ctx.create_window(Tilemap_Height, Tilemap_Width, "tiles");\
        TilemapBuffer tilebuffer;
        #endif

        bool running = true;
        GB::GameBoy device(game, [&](FrameBuffer& buf) {

            #ifndef NDEBUG
            debug_render_tileset(device, tilebuffer);
            if(!render_buffer(tile_window, tilebuffer)) {
                throw std::runtime_error("Unable to render tile buffer");
            };
            #endif
            
            if(!render_buffer(game_window, buf)) {
                throw std::runtime_error("Unable to render buffer");
            };

            for(Event_Type event : poll_events(ctx)) {
                switch(event) {
                case(Event_Type::QUIT):
                    running = false;
                }
            }
        });

        while(running) {
            device.run();
        }
    } catch(const std::exception& e) {
        std::cerr << "Fatal exception: " << e.what() << "\n";
        Log::log<Log::Level::Error>("Fatal exception: {}", e.what());

        return 1;
    }
}