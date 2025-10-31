// libraries
#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>

// headers
#include "event_handler.h"
#include "../pre_process/pre_process.h"
#include "../rotate/rotate.h"
#include "../segmentation/segmentation.h"

int event_handler(SDL_Renderer *renderer, SDL_Texture **texture, SDL_Surface **surface, int *steps, int *av_gray, char *file) 
{
	SDL_Event event;

	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_QUIT:

			printf("Exit\n");
			fflush(stdout);

			return 0;

			case SDL_KEYDOWN:
			
			if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)
			{
				switch (*steps)
				{

				case 0:

					int gray = 0;
					to_gray_scale(*surface, &gray);
					*av_gray = gray;

					printf("Grayscale Done\n");
					fflush(stdout);
					
					break;

				case 1:

					denoise(*surface);

					printf("Denoise Done\n");
					fflush(stdout);

					break;

				case 2:

					binarize(*surface, *av_gray);

					printf("Binarize Done\n");
					fflush(stdout);

					break;

				case 3:

					*surface = rotate(*surface);

					printf("Rotate Done\n");
					fflush(stdout);

					break;

				case 4:
					
					SDL_Surface *copy = SDL_ConvertSurfaceFormat(*surface, SDL_PIXELFORMAT_RGBA8888, 0);

					save_letters(copy, file);
					SDL_FreeSurface(copy);
					
					printf("Segmentation Done\n");
					fflush(stdout);

					break;

				default:

					printf("Exit\n");
					fflush(stdout);
					
					return 0;
				}

				SDL_Texture *updated = SDL_CreateTextureFromSurface(renderer, *surface);
				*texture = updated;

				(*steps)++;
			}
			else if(event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT)
			{
				double angle = event.key.keysym.sym == SDLK_LEFT ? -5.0 : 5.0;

				*surface = rotozoomSurface(*surface, angle);

				SDL_Texture *updated = SDL_CreateTextureFromSurface(renderer, *surface);
				*texture = updated;

				printf("Manual %s rotation Done\n", angle == -5.0 ? "left" : "right");
				fflush(stdout);
			}

			break;

			default:
			break;
		}
	}

	return 1;
}