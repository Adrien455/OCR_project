// libraries
#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// headers
#include "loader.h"
#include "../pre_process/pre_process.h"
#include "../rotate/rotate.h"
#include "../segmentation/segmentation.h"

void initialize(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture, char *file)
{	
	SDL_Init(SDL_INIT_VIDEO);

	*window = SDL_CreateWindow("Loader", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);

	*renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
	
	*texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 800, 600);

	if(!window || !renderer || !texture)
	{

        errx(EXIT_FAILURE, "%s", SDL_GetError());
  	}

	if(file != NULL) 
		{
		
		if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) 
		{

            errx(EXIT_FAILURE, "Failed to init image: %s", IMG_GetError());
        }

		SDL_Surface *surface = IMG_Load(file);

		if(!surface) 
		{
          	errx(EXIT_FAILURE, "Failed to load image: %s.\nPlease make sure the file exists and has a valid image extension.", SDL_GetError());
        }

		SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
		SDL_FreeSurface(surface);

		if(!converted)
        {
            errx(EXIT_FAILURE, "%s", SDL_GetError());
        }

		// Pre-process + segment + save letters
		// for now these steps are part of the loader, but they should be moved to main.c later and be manually done from there
		// (so that the user can choose when to do it, and could do his own rotation)

		int av_gray = 0;

		increaseContrast(converted);				// increase contrast (can take neg values btw)
		to_gray_scale(converted, &av_gray);			// convert to grayscale
		denoise(converted);							// denoise
		binarize(converted, av_gray);				// binarize with a fixed treshold (for now)
		converted = rotate(converted);				// rotate

		SDL_Surface *copy = SDL_ConvertSurfaceFormat(converted, SDL_PIXELFORMAT_RGBA8888, 0);

		save_letters(copy, file);				// segment and save letters (take a copy to not change black pixels to red)
		SDL_FreeSurface(copy);

		SDL_Texture *input = SDL_CreateTextureFromSurface(*renderer, converted);
		SDL_FreeSurface(converted);

		if(!input)
        {
			errx(EXIT_FAILURE, "%s", SDL_GetError());
		}

		*texture = input;
		
        SDL_RenderCopy(*renderer, *texture, NULL, NULL);
		SDL_RenderPresent(*renderer);	
	}

}

void terminate(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture) 
{
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyTexture(texture);
	SDL_Quit();
}

void save_bmp(SDL_Renderer *renderer)
{
        SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 800, 600, 32, SDL_PIXELFORMAT_RGBA8888);

        if(!surface) 
		{
          	errx(EXIT_FAILURE, "%s", SDL_GetError());
        }

        int i = SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);

        if(i != 0)
        {
            errx(EXIT_FAILURE, "%s", SDL_GetError());
        }

        SDL_SaveBMP(surface, "output.bmp");
        SDL_FreeSurface(surface);
}
