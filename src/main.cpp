#include <iostream>
#include <SDL2/SDL.h>

#include "cpu.h"

int main(int argc, char **argv) {

	if (argc < 2) {
		std::cout << "no file specified" << std::endl;
		return EXIT_FAILURE;
	}

	SDL_Init(SDL_INIT_EVERYTHING);
		
	SDL_Window *window = SDL_CreateWindow(
		"CHIP8 interpreter",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		VIDEO_WIDTH * VIDEO_MULTI,
		VIDEO_HEIGHT * VIDEO_MULTI,
		SDL_WINDOW_SHOWN
		//SDL_WINDOW_HIDDEN
	);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	
	if (window == NULL) {
		std::cout << "Could not create window: " << SDL_GetError() << std::endl;
		return 1;
	}

	Cpu cpu;
	cpu.load_rom(argv[1]);
	cpu.run(renderer);
	
	SDL_Delay(1000);	
	SDL_DestroyWindow(window);	
	SDL_Quit();

	return 0;
}

