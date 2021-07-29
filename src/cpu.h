#pragma once

#include <cstdint>
#include <stack>
#include <SDL2/SDL.h>

#define VIDEO_MULTI  10
#define VIDEO_WIDTH  64
#define VIDEO_HEIGHT 32

#define IS_NEGATIVE(value) ((bool)(value & (1 << 7)))
#define IS_ZERO(value) ((bool)(value == 0))

struct cpu_state {
    uint8_t ram[0x1000];
    uint8_t regs[0x10];

    bool video[VIDEO_WIDTH][VIDEO_HEIGHT];

    std::stack<uint16_t> call_stack;

    uint16_t pc, l;

    uint8_t delay_timer, sound_timer;
};

class Cpu {
public:
    struct cpu_state state;

    Cpu();
    void load_rom(char *path);
    void run(SDL_Renderer *renderer);

private:
    void mem_write(uint16_t ptr, uint8_t value);
    uint8_t mem_read(uint16_t ptr);

    void mem_write_u16(uint16_t ptr, uint16_t value);
    uint16_t mem_read_u16(uint16_t ptr);
};