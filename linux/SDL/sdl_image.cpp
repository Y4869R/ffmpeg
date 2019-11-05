#include <iostream>

#include "SDL.h"
#include "SDL_image.h"

int main() {
    printf("InitializingSDL.\n");
    if((SDL_Init(SDL_INIT_VIDEO)==-1))//判断是否进入SDL库
    {
       printf("Couldnot initialize SDL: %s.\n", SDL_GetError());
       return -1;
    }

    printf("SDLinitialized.\n");
    SDL_Surface*screen = NULL; // 创建一个窗口，并加载一张图片
    screen =SDL_SetVideoMode( 642, 482, 24, SDL_SWSURFACE ); //创建SDL执行窗口并设置像素点及位深
    SDL_Surface*img = NULL;
    img =SDL_LoadBMP( "muq.bmp" ); //装载位图
    SDL_BlitSurface(img, NULL, screen, NULL ); //块移图面
    SDL_Flip(screen );//显示加载的图片
    SDL_UpdateRect(screen, 0, 0, 0, 0);
    SDL_Delay(10000); 
    printf("QuitingSDL.\n");
    SDL_Quit();
    printf("Quiting....\n");

    return 0;
}
