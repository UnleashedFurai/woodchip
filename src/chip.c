#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define STACK_MAX 16
#define REGISTERS 16

const size_t RAM = 4096;        /* number of bytes in 4kb. size of RAM */
uint8_t *ramPtr;                /* pointer to our emulated RAM */
uint8_t *pc;                    /* our program counter */
uint16_t idx = 0;               /* the index register */
uint16_t stack[STACK_MAX];      /* array for stack */
uint8_t stack_top = -1;         /* index of current position in stack */
uint8_t registers[REGISTERS];   /* array holding our registers */
uint8_t delay_timer = 0;        /* delay timer; decrements at 60hz. */
uint8_t sound_timer = 0;        /* sound timer */
uint8_t font[80] = {            /* standard chip-8 font */
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

int stack_push(uint16_t in) {
    if (stack_top >= STACK_MAX) {
        printf("ERROR: Cannot push. Stack is full.\n");
        return -1;
    }

    stack[++stack_top]=in;
    return 0;
}

uint16_t stack_pop() {
    if (stack_top < 0) {
        printf("ERROR: Cannot pop. Stack is empty.\n");
        return -1;
    }

    return(stack[stack_top--]);
}

size_t filesize(FILE* f) {
    fseek(f, 0L, SEEK_END);
    size_t s = ftell(f);
    fseek(f, 0L, SEEK_SET);
    return s;
}

// TODO: implement
int decode(uint8_t op) {
    /*
    switch(op) {
        default:
            return -1;
    }
    */
    return 0;
}

uint16_t fetch() {
    uint16_t op = (pc[0] << 8) | pc[1];

    pc=pc+sizeof(uint16_t);
    return op;
}

// TODO: run n instructios per second
// maybe start around 700?
int chip_cycle() {
    uint16_t op = fetch();
    if(!op) {
        printf("ERROR: Failed to get instruction at: %p\n", pc);
        return -1;
    }

    if(decode(op) != 0) {
        printf("ERROR: Failed to decode instruction: %b\n", op);
        return -1;
    }

    // decremet timers
    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) {
        // TODO: play sound
        sound_timer--;
    }

    return 0;
}

int chip_init(char* filename) {
    ramPtr = malloc(RAM);
    if (!ramPtr) {
        printf("ERROR: Failed to allocate RAM.\n");
        return -1;
    }

    pc = ramPtr + 0x200;

    // load the rom
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("ERROR: File %s could not be loaded.\n", filename);
        return -1;
    }

    size_t size = filesize(f);

    size_t read = fread(pc, 1, size, f);
    if (read != size) {
        printf("ERROR: Failed to copy file to RAM.\n");
    }

    // unload file
    fclose(f);

    return 0;
}

int chip_clean() {
    free(ramPtr);

    return 0;
}
