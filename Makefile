all:
	g++ src/*.cpp -o chip8_emu -g -lSDL2 -Wall -Werror -fwrapv

run:
	@ ./chip8_emu ./rom/ibm_logo.ch8

test:
	@ ./chip8_emu ./rom/test_opcode.ch8
