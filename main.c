// libraries
#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>

// headers
#include "loader/loader.h"
#include "pre_process/pre_process.h"
#include "rotate/rotate.h"

int main(int argc, char *argv[]) {

        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
                errx(EXIT_FAILURE, "%s", SDL_GetError());
        }

        SDL_Renderer *renderer = NULL;
        SDL_Window *window = NULL;
        SDL_Texture *texture = NULL;

        char* file = NULL;
        if (argc >1)
        {
                file = argv[1];

        }

        initialize(&window, &renderer, &texture, file);

        int running = 1;

        while (running)
        {
        running = event_handler();
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        }

        terminate(window, renderer, texture);
        return EXIT_SUCCESS;
}
