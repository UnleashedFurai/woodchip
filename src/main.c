#include "chip.h"
#include "usage.h"

#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define CHIP_8_WIDTH                64      /* width of chip-8 in pixels */
#define CHIP_8_HEIGHT               32      /* height of chip-8 in pixels */
#define WINDOW_SIZE_MODIFIER        16      /* the amount the window size is multiplied by */

#define SDL_WINDOW_TITLE            "woodchip"
#define SDL_WINDOW_WIDTH            (CHIP_8_WIDTH * WINDOW_SIZE_MODIFIER)
#define SDL_WINDOW_HEIGHT           (CHIP_8_HEIGHT * WINDOW_SIZE_MODIFIER)

int* frame_buffer;
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

    return 0;
}

void program_loop() {
    int running = 0;
    while (running == 0) {
        while (SDL_PollEvent(&sdl_event)) {
            switch(sdl_event.type) {
                case SDL_EVENT_QUIT:
                    running = -1;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    switch (sdl_event.key.scancode) {
                        case SDL_SCANCODE_ESCAPE:
                            running = -1;
                            break;
                        default:
                            break;
                    }
                default:
                    break;
            }
        }
    }
}

/*
 * parse input in argv and act accordingly
 * successful parse returns 0.
 * no args or "-h" returns 1
 * failed parse returns -1
 */
int parse_input(char *argv[]) {
    // if no arguments, return immediately
    if (argv[1] == NULL) {
        print_usage();
        return 1;
    }

    int i=1;
    while(argv[i]!=NULL) {
        printf("argv[%d]: %s\n", i, argv[i]);
        if (strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 1;
        } else {
            printf("ERROR: Could not parse input.\n");
            print_usage();
            return -1;
        }
        i++;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int status = parse_input(argv);
    if(status < 0) {
        return -1;
    } else if (status > 0) {
        return 0;
    }

    if(init_sdl() != 0) {
        return -1;
    }

    if(chip_init() != 0) {
        chip_clean();
        return -1;
    }

    program_loop();

    chip_clean();
    return 0;
}
