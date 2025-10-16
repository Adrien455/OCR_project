#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "pre_process.h"

float contrastFactor(float averageBrightness)
{
	float factor = 1.5f;

	if (averageBrightness < 100) factor = 1.8f;		    // dark image
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

	return (Uint8)(sum / count);
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
	float factor = contrastFactor(computeAverageGray(surface));

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

void denoise(SDL_Surface* surface)
{
    int w = surface->w;
    int h = surface->h;
    
    Uint32* pixels = (Uint32*)surface->pixels;
    Uint32* copy = malloc(sizeof(Uint32) * w * h);
    memcpy(copy, pixels, sizeof(Uint32) * w * h);

    for (int y = 1; y < h-1; y++)
    {
        for (int x = 1; x < w-1; x++) 
        {
            Uint8 gray[9], r, g, b;
            int k = 0;
            
            for (int j = -1; j <= 1; j++) 
            {
                for (int i = -1; i <= 1; i++) 
                {
                    SDL_GetRGB(copy[(y+j)*w + (x+i)], surface->format, &r, &g, &b);
                    gray[k++] = r; 
                }
            }

            for (int m = 0; m < 8; m++)
             {
                for (int n = m+1; n < 9; n++) 
                {
                    if (gray[n] < gray[m]) 
                    {
                        Uint8 tmp = gray[m];
                        gray[m] = gray[n];
                        gray[n] = tmp;
                    }
                }
            }
            pixels[y*w + x] = SDL_MapRGB(surface->format, gray[4], gray[4], gray[4]);
        }
    }
    
    free(copy);
}