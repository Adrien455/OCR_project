#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "pre_process.h"

float averageBrightness(SDL_Surface *surface)
{
	Uint8 r, g, b;
	Uint32* pixels = (Uint32*)surface->pixels;
	int count = surface->w * surface->h;
	long sum = 0;

	for (int i = 0; i < count; i++) {
        	SDL_GetRGB(pixels[i], surface->format, &r, &g, &b);
        	Uint8 gray = (Uint8)(0.299*r + 0.587*g + 0.114*b);
        	sum += gray;
	}

	return (float)sum / count;
}

float contrastFactor(float averageBrightness)
{
	float factor = 1.5f;

	if (averageBrightness < 100) factor = 1.8f;		// dark image
	else if (averageBrightness > 180) factor = 1.2f;	// bright image

	return factor;
}

Uint8 computeAverageGray(SDL_Surface *surface)
{
	Uint8 r, g, b;
	Uint32* pixels = (Uint32*)surface->pixels;
	int count = surface->w * surface->h;
	long sum = 0;

	for (int i = 0; i < count; i++) 
	{
        	SDL_GetRGB(pixels[i], surface->format, &r, &g, &b);
        	Uint8 gray = (Uint8)(0.299*r + 0.587*g + 0.114*b);
		sum += gray;
	}

	return (Uint8)(sum / count);  // value between 0–255
}

void to_gray_scale(SDL_Surface *surface)
{
        Uint8 r, g, b;
        Uint32* pixels = (Uint32*)surface->pixels;
        int count = (surface->w * surface->h);

        for (int i = 0; i < count; i++)
        {
                SDL_GetRGB(pixels[i], surface->format, &r, &g, &b);
                Uint8 gray = (Uint8)(0.299 * r + 0.587 * g + 0.114 * b);
                pixels[i] = SDL_MapRGB(surface->format, gray, gray, gray);
        }
}

void binarize(SDL_Surface *surface)
{
	Uint8 threshold = computeAverageGray(surface);

        Uint8 r, g, b;
        Uint32* pixels = (Uint32*)surface->pixels;
        int count = (surface->w * surface->h);

        for (int i = 0; i < count; i++)
        {
                SDL_GetRGB(pixels[i], surface->format, &r, &g, &b);
                Uint8 value = (r > threshold) ? 255 : 0;
                pixels[i] = SDL_MapRGB(surface->format, value, value, value);
        }
}

void increaseContrast(SDL_Surface *surface)
{
	float factor = contrastFactor(averageBrightness(surface));

        Uint8 r, g, b;
        Uint32* pixels = (Uint32*)surface->pixels;
        int count = surface->w * surface->h;

        for (int i = 0; i < count; i++)
        {
                SDL_GetRGB(pixels[i], surface->format, &r, &g, &b);

                r = (Uint8) fmin(fmax((r - 128) * factor + 128, 0), 255);
                g = (Uint8) fmin(fmax((g - 128) * factor + 128, 0), 255);
                b = (Uint8) fmin(fmax((b - 128) * factor + 128, 0), 255);

                pixels[i] = SDL_MapRGB(surface->format, r, g, b);
        }
}
