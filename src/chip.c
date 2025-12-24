#include "macros.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define STACK_MAX 16
#define REGISTERS 16

const size_t RAM = 4096;            /* number of bytes in 4kb. size of RAM */
uint8_t *ram_ptr;                   /* pointer to our emulated RAM */
uint8_t *pc;                        /* our program counter */
uint8_t *font_ptr;                  /* points to font location in ram */
uint16_t idx = 0;                   /* the index register */
uint16_t stack[STACK_MAX];          /* array for stack */
int stack_top = -1;                 /* index of current position in stack */
uint8_t registers[REGISTERS] = {0}; /* array holding our registers */
uint8_t delay_timer = 0;            /* delay timer; decrements at 60hz. */
uint8_t sound_timer = 0;            /* sound timer */
uint8_t font[80] = {                /* standard chip-8 font */
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
uint8_t pixels[CHIP_8_WIDTH][CHIP_8_HEIGHT] = {0};
int keys[16] = {0};
int key_wait = 0;
int key_wait_filled = 1;
uint8_t *key_register;

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

int chip_clean() {
    free(ram_ptr);

    return 0;
}

int decode(uint16_t op) {
    // get each individual op
    uint8_t ops[4];
    ops[0] = (op & 0xF000) >> 12;
    ops[1] = (op & 0x0F00) >> 8;
    ops[2] = (op & 0x00F0) >> 4;
    ops[3] = (op & 0x000F) >> 0;

    switch(ops[0]) {
        case 0x0:
            switch(ops[2]) {
                case 0xE:
                    switch(ops[3]) {
                        case 0x0:
                            // 00E0
                            // clear the screen
                            memset(pixels, 0, sizeof(pixels));
                            break;
                        case 0xE:
                            // 00EE
                            // return from a subroutine
                            pc = ram_ptr + stack_pop();
                            break;
                        default:
                            return -1;
                    }
                    break;

                default:
                    // 0NNN
                    // execute machine language subroutine at address NNN
                    // not needed unless directly emulating COSMAC VIP, ETI-660, or DREAM 6800
                    return -1;
                    break;
            }
            break;

        case 0x1: {
            // 1NNN
            // jump to address NNN
            uint16_t offset = op & 0x0FFF;
            pc = ram_ptr + offset;
            break;
        }
            
        case 0x2: {
            // 2NNN
            // execute subroutine startign at address NNN
            stack_push((uint16_t)(pc-ram_ptr));
            uint16_t addr = op & 0x0FFF;
            pc = ram_ptr + addr;
            break;
        }

        case 0x3: {
            // 3XNN
            // skip the following instruction if the value of VX equals NN
            uint8_t nn = op & 0x00FF;
            if (registers[ops[1]] == nn) pc+=sizeof(uint16_t);
            break;
        }

        case 0x4: {
            // 4XNN
            // skip the following instruction if the value of VX is nnont equal to NN
            uint8_t nn = op & 0x00FF;
            if (registers[ops[1]] != nn) pc += sizeof(uint16_t);
            break;
        }

        case 0x5: {
            // 5XY0
            // skip the following instructionn if the value of VX is equal to the value of VY
            if (registers[ops[1]] == registers[ops[2]]) pc += sizeof(uint16_t);
            break;
        }

        case 0x6: {
            // 6XNN
            // store NN in VX
            uint8_t nn = op & 0x00FF;
            registers[ops[1]] = nn;
            break;
        }

        case 0x7: {
            // 7XNN
            // add NN to VX
            uint8_t nn = op & 0x00FF;
            registers[ops[1]] += nn;
            break;
        }

        case 0x8:
            switch(ops[3]) {
                case 0x0:
                    // 8XY0
                    // store the value of VY in VX
                    registers[ops[1]] = registers[ops[2]];
                    break;
                
                case 0x1:
                    // 8XY1
                    // set VX to VX OR VY
                    registers[ops[1]] = registers[ops[1]] | registers[ops[2]];
                    break;

                case 0x2:
                    // 8XY2
                    // set VX to VX AND VY
                    registers[ops[1]] = registers[ops[1]] & registers[ops[2]];
                    break;

                case 0x3:
                    // 8XY3
                    // set VX to VX XOR VY
                    registers[ops[1]] = registers[ops[1]] ^ registers[ops[2]];
                    break;

                case 0x4: {
                    // 8XY4
                    // add the value of VY to VX
                    // set VF to 01 if a carry occurs
                    // set VF to 00 if no carry occurs
                    uint8_t tmp_vx = registers[ops[1]];
                    registers[ops[1]] += registers[ops[2]];

                    if (registers[ops[1]] <= tmp_vx) registers[0xF] = 1;
                    else registers[0xF] = 0;
                    break; 
                }

                case 0x5:
                    // 8XY5
                    // subtract the value of VY from VX
                    // set VF to 00 if a borrow occurs
                    // set VF to 01 if no borrow occurs
                    registers[0xF] = (registers[ops[1]] >= registers[ops[2]]) ? 1 : 0;
                    registers[ops[1]] -= registers[ops[2]];
                    break;

                case 0x6:
                    // 8XY6
                    // store the value of VY shifted right one bit in VX
                    // set VF to the least significant bit prior to the shift
                    // VY is unchanged
                    registers[0xF] = registers[ops[2]] & 1;
                    registers[ops[1]] = registers[ops[2]] >> 1;
                    break;

                case 0x7:
                    // 8XY7
                    // set VX to the value of VY minux VX
                    // set VF to 00 if a borrow occurs
                    // set VF to 01 if a no borrow occurs
                    registers[0xF] = (registers[ops[2]] >= registers[ops[1]]) ? 1 : 0;
                    registers[ops[1]] =  registers[ops[2]] - registers[ops[1]];
                    break;

                case 0xE:
                    // 8XYE
                    // store the value of VY shifted left one bit in VX
                    // set VF to the most significant bit prior to the shift
                    // VY is unchanged
                    // 0b10000000 -> 0x80
                    registers[0xF] = registers[ops[2]] & 0x80;
                    registers[ops[1]] = registers[ops[2]] << 1;
                    break;

                default:
                    return -1;
            }
            break;

        case 0x9:
            switch(ops[3]) {
                case 0x0:
                    // 9XY0
                    // skip the folowing instruction if the value of VX is not equal to the value of VY
                    if (registers[ops[1]] != registers[ops[2]]) pc+=sizeof(uint16_t);
                    break;

                default:
                    // illegal instruction
                    return -1;
            }
            break;

        case 0xA:
            // ANNN
            // store memory address NNN in index
            idx = op & 0x0FFF;
            break;
            
        case 0xB: {
            // BNNN
            // jump to address NNN + V0
            uint16_t offset = op & 0x0FFF;
            pc = ram_ptr + (registers[0] + offset);
            break;
        }

        case 0xC: {
            // CXNN
            // set VX to a random number with a mask of NN
            uint8_t nn = op & 0x00FF;
            registers[ops[1]] = nn & rand();
            break;
        }

        case 0xD: {
            // DXYN
            // draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in index
            // set VF to 01 if any pixels are changed to unset, 00 otherwise
            int vx = registers[ops[1]];
            int vy = registers[ops[2]];

            registers[0xF] = 0;

            for(int i=0; i<ops[3]; i++) {
                uint8_t sprite = *(ram_ptr + idx + i);
                for(int j=0; j<8; j++) {
                    uint8_t pixel = (sprite >> (7-j)) & 0x1;
                    if (pixel == 0) continue;

                    int x = (vx+j) % CHIP_8_WIDTH;
                    int y = (vy+i) % CHIP_8_HEIGHT;

                    if (pixels[x][y] == 1)
                        registers[0xF] = 1;
                    pixels[x][y] ^= 1;
                }
            }
            // break;
            return 1;
        }

        case 0xE:
            switch(ops[2]) {
                case 0x9:
                    // EX9E
                    // skip the following instruction if the key corresponding to the hex value curretly stored in VX is pressed
                    if (keys[registers[ops[1]]]) pc += sizeof(uint16_t);
                    break;

                case 0xA:
                    // EXA1
                    // skip the followig instruction if the key corresponding to the hex value currently stored in VX is not pressed
                    if (!keys[registers[ops[1]]]) pc += sizeof(uint16_t);
                    break;

                default:
                    // illegal instruction
                    return -1;
            }
            break;

        case 0xF:
            switch(ops[2]) {
                case 0x0:
                    switch(ops[3]) {
                        case 0x7:
                            // FX07
                            // store the current value of the delay timer in VX
                            registers[ops[1]] = delay_timer;
                            break;

                        case 0xA:
                            // FX0A
                            // wait for a keypress and store the result in VX
                            if (key_wait && !key_wait_filled) {
                                /*
                                 * should already have this state:
                                 * key_wait = 1;
                                 * key_wait_filled = 0;
                                 * key_register = &registers[ops[1]];
                                 */
                                pc -= sizeof(uint16_t);
                            } else if (key_wait && key_wait_filled) {
                                // key was pressed, register was filled
                                key_wait = 0;
                                key_wait_filled = 0;
                                key_register = NULL;
                            } else {
                                // initialize key wait
                                key_wait = 1;
                                key_wait_filled = 0;
                                key_register = &registers[ops[1]];
                                pc -= sizeof(uint16_t);
                            }
                            break;

                        default:
                            // illegal instruction
                            return -1;
                    }
                    break;

                case 0x1:
                    switch(ops[3]) {
                        case 0x5:
                            // FX15
                            // set the delay timer to the value of VX
                            delay_timer = registers[ops[1]];
                            break;
                        
                        case 0x8:
                            // FX18
                            // set the sound timer t the value of register VX
                            sound_timer = registers[ops[1]];
                            break;

                        case 0xE: {
                            // FX1E
                            // add the value stred in VX to index
                            uint16_t idx_tmp = idx;
                            idx += registers[ops[1]];
                            if (idx_tmp > idx) registers[0xF] = 1;
                            break;
                        }

                        default:
                            // illegal instruction
                            return -1;
                    }
                    break;

                case 0x2:
                    switch(ops[3]) {
                        case 0x9:
                            // FX29
                            // set index to the memory address of the sprite data corresponding to the hexademical digit stored in VX
                            idx = (font_ptr + ops[1]*5) - ram_ptr;
                            break;

                        default:
                            // illegal instruction
                            return -1;
                        }
                    break;

                case 0x3:
                    switch(ops[3]) {
                        case 0x3: {
                            // FX33
                            // store the binary-coded decimal equivalent of the value stored in VX at addresses idnex, idex+1, index+2
                            int val = registers[ops[1]];
                            for (int i=2; i>=0; i--) {
                                *(ram_ptr + idx + i) = val % 10;
                                val /= 10;
                            }
                            break;
                        }

                        default:
                            // illegal instruction
                            return -1;
                        }
                    break;

                case 0x5:
                    switch(ops[3]) {
                        case 0x5: {
                            // FX55
                            // store the values of V0-VX inclusive in memory starting at index
                            // index is set to idnex + X + 1 after operation
                            for (int i=0; i<=ops[1]; i++)
                                *(ram_ptr + idx + i) = registers[i];
                            break;
                        }

                        default:
                            // illegal instruction
                            return -1;
                    }
                    break;

                case 0x6:
                    switch(ops[3]) {
                        case 0x5:
                            // FX65
                            // fill registers V0-VX inclusive with the values stored in memory starting at index
                            // index is set to index + x + 1 after operation
                            for (int i=0; i<ops[1]; i++) {
                                registers[ops[1]] = *(ram_ptr + idx);
                                idx++;
                            }
                            break;

                        default:
                            // illegal instructio
                            return -1;
                    }
                    break;

                default:
                    // illegal instruction
                    return -1;
            }
            break;

        default:
            // illegal instruction
            return -1;
    }
    return 0;
}

uint16_t fetch() {
    uint16_t op = (pc[0] << 8) | pc[1];

    pc += sizeof(uint16_t);
    return op;
}

int decrement_timers() {
    if (delay_timer > 0) delay_timer --;
    if (sound_timer > 0) sound_timer --;
    return 0;
}

// returns 1 if screen needs to be updated
int chip_cycle() {
    uint16_t op = fetch();

    int decode_status = decode(op);
    if(decode_status < 0) {
        printf("ERROR: Failed to decode instruction: %x\n", op);
        return -1;
    }

    // decremet timers
    if (sound_timer > 0) {
        // TODO: play sound
    }

    return (decode_status == 1) ? 1 : 0;
}

int chip_init(char* filename) {
    ram_ptr = malloc(RAM);
    if (!ram_ptr) {
        printf("ERROR: Failed to allocate RAM.\n");
        return -1;
    }

    pc = ram_ptr + 0x200;
    font_ptr = ram_ptr + 0x50;

    // load the font
    memcpy(font_ptr, font, sizeof(font));

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
