#ifndef CHIP
#define CHIP

#include "macros.h"
#include <stdint.h>

extern uint8_t pixels[CHIP_8_WIDTH][CHIP_8_HEIGHT];
extern int keys[16];
extern int key_wait_filled;
extern uint8_t *key_register;

int decrement_timers();
int chip_cycle();
int chip_init(char* file);
int chip_clean();

#endif
