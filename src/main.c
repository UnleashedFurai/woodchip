#include "chip.h"
#include "usage.h"
#include "macros.h"

#include <stdint.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

SDL_Window* sdl_window;
SDL_Renderer* sdl_renderer;
SDL_Event sdl_event;

int init_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        printf("ERROR: Failed to initialize SDL3: %s\n", SDL_GetError());
        return -1;
    }

    sdl_window = SDL_CreateWindow(SDL_WINDOW_TITLE, SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, 0);
    if (!sdl_window) {
        printf("ERROR:  Failed to create window: %s\n", SDL_GetError());
        return -1;
    }

    sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
    if (!sdl_renderer) {
        printf("ERROR: Failed to create renderer: %s\n", SDL_GetError());
        return -1;
    }

    int vsync = SDL_SetRenderVSync(sdl_renderer, 1);
    if (vsync != 1) {
        printf("ERROR: Failed to initialize VSync: %s\n", SDL_GetError());
        return -1;
    }

    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderPresent(sdl_renderer);

    return 0;
}

int destroy_sdl() {
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    return 0;
}

int scan_to_chip(SDL_Scancode s) {
    switch (s) {
        /*
         * the key layout is as follows
         *     qwerty    ->      chip-8
         *   1  2  3  4        1  2  3  C
         *   Q  W  E  R        4  5  6  D
         *   A  S  D  F        7  8  9  E
         *   Z  X  C  V        A  0  B  F
         */
        case SDL_SCANCODE_1: return 0x1;
        case SDL_SCANCODE_2: return 0x2;
        case SDL_SCANCODE_3: return 0x3;
        case SDL_SCANCODE_4: return 0xC;

        case SDL_SCANCODE_Q: return 0x4;
        case SDL_SCANCODE_W: return 0x5;
        case SDL_SCANCODE_E: return 0x6;
        case SDL_SCANCODE_R: return 0xD;

        case SDL_SCANCODE_A: return 0x7;
        case SDL_SCANCODE_S: return 0x8;
        case SDL_SCANCODE_D: return 0x9;
        case SDL_SCANCODE_F: return 0xE;

        case SDL_SCANCODE_Z: return 0xA;
        case SDL_SCANCODE_X: return 0x0;
        case SDL_SCANCODE_C: return 0xB;
        case SDL_SCANCODE_V: return 0xF;

        default: return -1;
    }
}

void draw_screen() {
    SDL_RenderClear(sdl_renderer);
    for (int i=0; i<CHIP_8_WIDTH; i++) {
        for (int j=0; j<CHIP_8_HEIGHT; j++) {
            if (pixels[i][j] == 1) {
                SDL_SetRenderDrawColor(sdl_renderer, WHITE);
            } else {
                SDL_SetRenderDrawColor(sdl_renderer, BLACK);
            }
            SDL_FRect f = {i * WINDOW_SIZE_MODIFIER, j * WINDOW_SIZE_MODIFIER, WINDOW_SIZE_MODIFIER, WINDOW_SIZE_MODIFIER};
            SDL_RenderFillRect(sdl_renderer, &f);
        }
    }
    SDL_RenderPresent(sdl_renderer);
}

void program_loop() {
    int running = 0;
    while (running == 0) {
        uint64_t render_start = SDL_GetTicksNS();
        while (SDL_PollEvent(&sdl_event)) {

            switch(sdl_event.type) {
                case SDL_EVENT_QUIT:
                    running = -1;
                    break;

                case SDL_EVENT_KEY_DOWN: {
                    int key = scan_to_chip(sdl_event.key.scancode);
                    if (key != -1) {
                        keys[key] = 1;
                        if (!key_wait_filled) {
                            *key_register = (uint8_t) key;
                            key_wait_filled = 1;
                        }
                    }
                    break;
                }

                case SDL_EVENT_KEY_UP: {
                    int key = scan_to_chip(sdl_event.key.scancode);
                    if (key != -1)
                        keys[key] = 0;
                    break;
                }
                    
                default:
                    break;
            }
        }
        
        int draw = 0;
        for (int i=0; i<CHIP_8_CYCLES_PER_FRAME; i++)
            if (chip_cycle()) draw = 1;
        if (draw) draw_screen();
        decrement_timers();

        // cap at 60FPS
        uint64_t render_time = SDL_GetTicksNS() - render_start;
        uint64_t frame_time = 1000000000 / SDL_FPS;
        if (render_time < frame_time) {
            uint64_t sleep = frame_time - render_time;
            SDL_DelayNS(sleep);
        }

        /*
        uint64_t fps = 10000000 / render_time;
        printf("fps: %ld\n", fps);
        */
    }
}

// TODO: add more commandline args
// consider for fullscreen, window size, and instructions per second
int main(int argc, char *argv[]) {
    char *file;
    // if no arguments, return immediately
    if (argc == 1) {
        print_usage();
        return 1;
    }

    // loop until second-last value. 
    // argv[argc-1] is the rom
    for(int i = 1; i < argc-1; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        }
    }

    file = argv[argc-1];
    printf("Loading file: %s\n", file);

    if(init_sdl() != 0) {
        return -1;
    }

    if(chip_init(file) != 0) {
        chip_clean();
        return -1;
    }

    program_loop();

    destroy_sdl();
    chip_clean();
    return 0;
}
