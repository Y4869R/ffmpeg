#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"

#include <iostream>

int main()
{
    int result  = SDL_Init(SDL_INIT_EVERYTHING);

    if(result == 0)
	std::cout << "init success" << std::endl;
    else
	std::cout << "init fail" << std::endl;

    SDL_Quit();
    return 0;
}
