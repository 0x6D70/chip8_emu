#include "cpu.h"
#include <iostream>
#include <assert.h>
#include <filesystem>
#include <SDL2/SDL.h>
#include <cstdlib>

void Cpu::mem_write(uint16_t ptr, uint8_t value) {
    this->state.ram[ptr] = value;
}

uint8_t Cpu::mem_read(uint16_t ptr) {
    return this->state.ram[ptr];
}

void Cpu::mem_write_u16(uint16_t ptr, uint16_t value) {
    this->mem_write(ptr+1, value & 0xff);
    this->mem_write(ptr, value >> 8);
}

uint16_t Cpu::mem_read_u16(uint16_t ptr) {
    return (this->mem_read(ptr) << 8)
          | this->mem_read(ptr+1);
}

Cpu::Cpu() {
    // clear ram
    for (int i = 0; i < 0x1000; i++) {
        this->state.ram[i] = 0;
    }

    // clear screen
    for (int y = 0; y < VIDEO_HEIGHT; y++) {
        for (int x = 0; x < VIDEO_WIDTH; x++) {
            this->state.video[x][y] = false;
        }
    }

    // load font
    unsigned char chip8_fontset[80] =
    {
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

    for (int i = 0; i < 80; i++) {
        this->state.ram[0x50 + i] = chip8_fontset[i];
    }
}


void Cpu::load_rom(char *path) {
    FILE *fp = fopen(path, "rb");
    std::filesystem::path p{path};
    fread(&this->state.ram[0x200], 1, (size_t)std::filesystem::file_size(p), fp);
    fclose(fp);

    this->state.pc = 0x200;
}

// keyboard layout:

// 1 2 3 4
// Q W E R
// A S D F
// Z X C V

uint8_t keysym_to_index(SDL_Keysym keysym) {
    switch (keysym.sym) {
        case SDLK_1:
            return 0x1;
        case SDLK_2:
            return 0x2;
        case SDLK_3:
            return 0x3;
        case SDLK_4:
            return 0xc;
        
        case SDLK_q:
            return 0x4;
        case SDLK_w:
            return 0x5;
        case SDLK_e:
            return 0x6;
        case SDLK_r:
            return 0xd;
        
        case SDLK_a:
            return 0x7;
        case SDLK_s:
            return 0x8;
        case SDLK_d:
            return 0x9;
        case SDLK_f:
            return 0xe;
        
        case SDLK_z:
            return 0xa;
        case SDLK_x:
            return 0x0;
        case SDLK_c:
            return 0xb;
        case SDLK_v:
            return 0xf;
        
        default:
            return 0xff;
    }
}

bool isKeyPressed(bool keys[0x10]) {
    for (int i = 0; i < 0x10; i++) {
        if (keys[i])
            return true;
    }

    return false;
}

void Cpu::run(SDL_Renderer *renderer) {

    bool isFinished = false;
    bool update_video = false;
    uint64_t instruction_count = 0;

    SDL_Event event;

    // variable for keyboard
    bool keys[0x10] = {false};
    

    while (!isFinished) {
        instruction_count++;

        uint16_t opcode = this->mem_read_u16(this->state.pc);
        this->state.pc += 2;

        //  uncomment for single stepping
        // std::cout << "0x" << std::hex << opcode;
        // getchar();

        uint8_t opcode_n3 = opcode & 0xf;
        uint8_t opcode_n2 = (opcode >> 4) & 0xf;
        uint8_t opcode_n1 = (opcode >> 8) & 0xf;
        uint8_t opcode_n0 = (opcode >> 12) & 0xf;

        switch (opcode_n0) {
            case 0x0: {
                if (opcode == 0x00e0) {
                    // cls
                    update_video = true;
                    for (int y = 0; y < VIDEO_HEIGHT; y++) {
                        for (int x = 0; x < VIDEO_WIDTH; x++) {
                            this->state.video[x][y] = false;
                        }
                    }
                } else if (opcode == 0x00ee) {
                    // ret
                    this->state.pc = this->state.call_stack.top();
                    this->state.call_stack.pop();
                } else {
                    // sys addr
                    // this instruction is ignored
                    // http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#0nnn
                }
                break;
            }
            case 0x1: {
                // JP addr
                this->state.pc = opcode & 0xfff;
                break;
            }
            case 0x2: {
                // CALL addr
                this->state.call_stack.push(this->state.pc);
                this->state.pc = opcode & 0xfff;
                break;
            }
            case 0x3: {
                // SE Vx, byte
                if (this->state.regs[opcode_n1] == (uint8_t)(opcode & 0xff)) {
                    // skip instruction
                    this->state.pc += 2;
                }
                break;
            }
            case 0x4: {
                // SNE Vx, byte
                if (this->state.regs[opcode_n1] != (uint8_t)(opcode & 0xff)) {
                    // skip instruction
                    this->state.pc += 2;
                }
                break;
            }
            case 0x5: {
                // SE Vx, Vy
                if (this->state.regs[opcode_n1] == this->state.regs[opcode_n2]) {
                    // skip instruction
                    this->state.pc += 2;
                }
                break;
            }
            case 0x6: {
                // ld vx,byte
                this->state.regs[opcode_n1] = (uint8_t)(opcode & 0xff);
                break;
            }
            case 0x7: {
                // ADD Vx, byte
                this->state.regs[opcode_n1] += (opcode & 0xff);
                break;
            }
            case 0x8: {
                // logical and arithmetic instruction
                switch (opcode_n3) {
                    case 0x0: {
                        this->state.regs[opcode_n1] = this->state.regs[opcode_n2];
                        break;
                    }
                    case 0x1: {
                        this->state.regs[opcode_n1] |= this->state.regs[opcode_n2];
                        break;
                    }
                    case 0x2: {
                        this->state.regs[opcode_n1] &= this->state.regs[opcode_n2];
                        break;
                    }
                    case 0x3: {
                        this->state.regs[opcode_n1] ^= this->state.regs[opcode_n2];
                        break;
                    }
                    case 0x4: {
                        // check overflow
                        if ((uint16_t)this->state.regs[opcode_n1] + (uint16_t)this->state.regs[opcode_n2] > 0xff) {
                            this->state.regs[0xf] = 1;
                        } else {
                            this->state.regs[0xf] = 0;
                        }

                        this->state.regs[opcode_n1] += this->state.regs[opcode_n2];
                        break;
                    }
                    case 0x5: {
                        if (this->state.regs[opcode_n1] > this->state.regs[opcode_n2]) {
                            this->state.regs[0xf] = 1;
                        } else {
                            this->state.regs[0xf] = 0;
                        }

                        this->state.regs[opcode_n1] -= this->state.regs[opcode_n2];
                        break;
                    }
                    case 0x6: {
                        this->state.regs[opcode_n1] = this->state.regs[opcode_n2];
                        // shift right

                        this->state.regs[0xf] = this->state.regs[opcode_n1] & 1;
                        this->state.regs[opcode_n1] >>= 1;

                        break;
                    }
                    case 0x7: {
                        if (this->state.regs[opcode_n2] > this->state.regs[opcode_n1]) {
                            this->state.regs[0xf] = 1;
                        } else {
                            this->state.regs[0xf] = 0;
                        }
                        
                        this->state.regs[opcode_n1] = this->state.regs[opcode_n2] - this->state.regs[opcode_n1];
                        break;
                    }
                    case 0xe: {
                        this->state.regs[opcode_n1] = this->state.regs[opcode_n2];
                        // shift left

                        this->state.regs[0xf] = (this->state.regs[opcode_n1] >> 7) & 1;
                        this->state.regs[opcode_n1] <<= 1;

                        break;
                    }
                }
                break;
            }
            case 0x9: {
                // SNE Vx, Vy
                if (this->state.regs[opcode_n1] != this->state.regs[opcode_n2]) {
                    // skip instruction
                    this->state.pc += 2;
                }
                break;
            }
            case 0xa: {
                // ld l, addr
                this->state.l = opcode & 0xfff;
                break;
            }
            case 0xb: {
                // Bnnn - JP V0, addr
                this->state.pc = (opcode & 0xfff) + this->state.regs[0x0];
                break;
            }
            case 0xc: {
                // Cxkk - RND Vx, byte
                this->state.regs[opcode_n1] = (uint8_t)(rand() & (opcode & 0xff));
                break;
            }
            case 0xd: {
                // DXYN
                uint8_t x_base = this->state.regs[opcode_n1] % 64;
                uint8_t y_base = this->state.regs[opcode_n2] % 32;
                this->state.regs[0xf] = 0;
                update_video = true;

                for (int n = 0; n < opcode_n3; n++) {
                    uint8_t pixels = this->state.ram[this->state.l + n];

                    for (int x = 0; x < 8; x++) {
                        if (pixels & (0x80 >> x)) {
                            // current bit is one

                            if (this->state.video[x_base + x][y_base + n]) {
                                this->state.regs[0xf] = 1;
                                // flip bit
                                this->state.video[x_base + x][y_base + n] = false;
                            } else {
                                this->state.video[x_base + x][y_base + n] = true;
                            }
                        }
                    }

                }

                break;
            }
            case 0xe: {
                switch (opcode & 0xff) {
                    case 0x9e: {
                        // Ex9E - SKP Vx
                        uint8_t index = this->state.regs[opcode_n1] & 0xf;
                        if (keys[index])
                            this->state.pc += 2;
                        break;
                    }
                    case 0xa1: {
                        // ExA1 - SKNP Vx
                        uint8_t index = this->state.regs[opcode_n1] & 0xf;
                        if (!keys[index])
                            this->state.pc += 2;
                        break;
                    }
                }
                break;
            }
            case 0xf: {

                switch (opcode & 0xff) { 
                    case 0x7: {
                        // LD Vx, DT
                        this->state.regs[opcode_n1] = this->state.delay_timer;
                        break;
                    }
                    case 0xa: {
                        // LD Vx, K
                        // wait until key is pressed
                        // when the key is pressed, put it into Vx

                        if (isKeyPressed(keys)) {
                            for (int i = 0; i < 0x10; i++) {
                                if (keys[i]) {
                                    this->state.regs[opcode_n1] = i;
                                }
                            }
                        } else {
                            // decrement pc to hit this instruction again
                            this->state.pc -= 2;
                        }
                        break;
                    }
                    case 0x15: {
                        // LD DT, Vx
                        this->state.delay_timer = this->state.regs[opcode_n1];
                        break;
                    }
                    case 0x18: {
                        // LD ST, Vx
                        this->state.sound_timer = this->state.regs[opcode_n1];
                        break;
                    }
                    case 0x1e: {
                        // ADD I, Vx
                        this->state.l += this->state.regs[opcode_n1];

                        if (this->state.l > 0xfff) {
                            this->state.regs[0xf] = 1;
                        } else {
                            this->state.regs[0xf] = 0;
                        }

                        this->state.l &= 0xfff;
                        break;
                    }
                    case 0x29: {
                        // LD F, Vx
                        this->state.l = 0x50 + (opcode & 0xff) * 5;
                        break;
                    }
                    case 0x33: {
                        // Fx33 - LD B, Vx
                        // Binary-coded decimal conversion

                        this->state.ram[this->state.l + 0] = this->state.regs[opcode_n1] / 100;
                        this->state.ram[this->state.l + 1] = (this->state.regs[opcode_n1] / 10) % 10;
                        this->state.ram[this->state.l + 2] = this->state.regs[opcode_n1] % 10;

                        break;
                    }
                    case 0x55: {
                        // LD [I], Vx

                        for (int i = 0; i <= opcode_n1; i++) {
                            this->state.ram[this->state.l + i] = this->state.regs[i];
                        }

                        break;
                    }
                    case 0x65: {
                        // LD Vx, [I]

                        for (int i = 0; i <= opcode_n1; i++) {
                            this->state.regs[i] = this->state.ram[this->state.l + i];
                        }

                        break;
                    }
                }
                break;
            }
            default:
                std::cout << "missing instruction 0x" << std::hex << (int)opcode_n0 << std::endl;
                isFinished = true;
        }

        // update timers

        if (instruction_count % 10  == 0) {
            // ownly decrease every 10 instructions which should be a rate of about ~60 times a second

            if (this->state.delay_timer > 0)
                this->state.delay_timer--;
            if (this->state.sound_timer > 0)
                this->state.sound_timer--;
        }

        if (update_video || instruction_count % 0x100 == 0) {
            // video array has changed => update window

            SDL_RenderClear(renderer);

            for (int y = 0; y < VIDEO_HEIGHT; y++) {
                for (int x = 0; x < VIDEO_WIDTH; x++) {
                    if (this->state.video[x][y]) {
                        // white
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    } else {
                        // black
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    }

                    SDL_Rect rect;
                    rect.h = VIDEO_MULTI;
                    rect.w = VIDEO_MULTI;
                    rect.x = x * VIDEO_MULTI;
                    rect.y = y * VIDEO_MULTI;

                    SDL_RenderFillRect(renderer, &rect);
                }
            }

            SDL_RenderPresent(renderer);

            update_video = false;
        }

        while(SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    isFinished = true;
                    break;
                case SDL_KEYDOWN: {
                    uint8_t index = keysym_to_index(event.key.keysym);

                    if (index < 0x10) {
                        keys[index] = true;
                    }
                    
                    break;
                }
                case SDL_KEYUP: {
                    uint8_t index = keysym_to_index(event.key.keysym);

                    if (index < 0x10) {
                        keys[index] = false;
                    }

                    break;
                }
            }
        }

        SDL_Delay(1);
    }

    std::cout << "Exited after 0x" << std::hex << (long)instruction_count << " instructions\n";
}