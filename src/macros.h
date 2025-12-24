#ifndef MACROS
#define MACROS

#define CHIP_8_WIDTH                64      /* width of chip-8 in pixels */
#define CHIP_8_HEIGHT               32      /* height of chip-8 in pixels */

#define SDL_WINDOW_TITLE            "woodchip"
#define SDL_WINDOW_WIDTH            (CHIP_8_WIDTH * WINDOW_SIZE_MODIFIER)
#define SDL_WINDOW_HEIGHT           (CHIP_8_HEIGHT * WINDOW_SIZE_MODIFIER)
#define SDL_FPS                     60      /* we run at 60 fps, the same speed as the chip-8 timers */
#define BLACK                       0 ,0 ,0 ,255
#define WHITE                       255, 255, 255, 255

#endif
