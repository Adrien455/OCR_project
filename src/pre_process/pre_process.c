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
	Uint32* pixels = (Uint32*)surface -> pixels;
	int count = surface -> w * surface -> h;
	long sum = 0;

	for (int i = 0; i < count; i++) 
	{
        SDL_GetRGB(pixels[i], surface -> format, &r, &g, &b);
        Uint8 gray = (Uint8)(0.299 * r + 0.587 * g + 0.114 * b);
		sum += gray;
	}

	return (Uint8)(sum / count);
}

void to_gray_scale(SDL_Surface *surface, int *av_gray)
{
        Uint8 r, g, b;
        Uint32* pixels = (Uint32*)surface -> pixels;

        int w = surface -> w;
        int h = surface -> h;

        long sum = 0;

        int pitch = surface -> pitch / 4;

        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                SDL_GetRGB(pixels[y * pitch + x], surface -> format, &r, &g, &b);
                Uint8 gray = (Uint8)(0.299 * r + 0.587 * g + 0.114 * b);
                pixels[y * pitch + x] = SDL_MapRGB(surface -> format, gray, gray, gray);
                sum += gray;
            }
        }

        *av_gray = (int)(sum / (w * h));
}

void binarize(SDL_Surface *surface, int av_gray)
{
        Uint8 r, g, b;
        Uint32* pixels = (Uint32*)surface -> pixels;
        
        int w = surface -> w;
        int h = surface -> h;

        int pitch = surface -> pitch / 4;

        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                SDL_GetRGB(pixels[y * pitch + x], surface -> format, &r, &g, &b);
                Uint8 value = (r > av_gray) ? 255 : 0;
                pixels[y * pitch + x] = SDL_MapRGB(surface -> format, value, value, value);
            }
        }
}

void increaseContrast(SDL_Surface *surface)
{
	float factor = contrastFactor(computeAverageGray(surface));

        Uint8 r, g, b;
        Uint32* pixels = (Uint32*)surface -> pixels;
        int count = surface -> w * surface -> h;

        for (int i = 0; i < count; i++)
        {
                SDL_GetRGB(pixels[i], surface -> format, &r, &g, &b);

                r = (Uint8) fmin(fmax((r - 128) * factor + 128, 0), 255);
                g = (Uint8) fmin(fmax((g - 128) * factor + 128, 0), 255);
                b = (Uint8) fmin(fmax((b - 128) * factor + 128, 0), 255);

                pixels[i] = SDL_MapRGB(surface->format, r, g, b);
        }
}

int check_line(Uint32* pixels, SDL_PixelFormat* format, int x, int y, int pitch) // check if pixel at (x,y) is part of a line (horizontal or vertical)
{
    Uint8 r1, r2;
    Uint8 g1, g2;
    Uint8 b1, b2;

    SDL_GetRGB(pixels[y * pitch + (x - 1)], format, &r1, &g1, &b1); // left and right
    SDL_GetRGB(pixels[y * pitch + (x + 1)], format, &r2, &g2, &b2);

    if(r1 != 255 && r2 != 255) return 1; // horizontal line (grayscaled so r=g=b)

    SDL_GetRGB(pixels[(y - 1)* pitch + x], format, &r1, &g1, &b1); // top and bottom
    SDL_GetRGB(pixels[(y + 1)* pitch + x], format, &r2, &g2, &b2);

    if(r1 != 255 && r2 != 255) return 1; // vertical line

    return 0;
}

void denoise(SDL_Surface* surface)  // simple denoise function using median filter, will compare the 3x3 neighborhood and take the median value
{
    int w = surface -> w;
    int h = surface -> h;

    Uint32* pixels = (Uint32*)surface -> pixels;

    Uint32* copy = malloc(sizeof(Uint32) * w * h);  // do copy
    memcpy(copy, pixels, sizeof(Uint32) * w * h);

    int pitch = surface -> pitch / 4;

    for (int y = 1; y < h - 1; y++)     // avoid borders
    {
        for (int x = 1; x < w - 1; x++)
        {
            if(copy[y * pitch + x] == SDL_MapRGB(surface -> format, 255, 255, 255)) continue; // skip white pixels

            if(check_line(copy, surface -> format, x, y, pitch) == 1) continue; // skip if part of a line

            Uint8 gray[9], r, g, b;     // 3x3 neighborhood
            int k = 0;
            
            for (int j = -1; j <= 1; j++)   // fill neighborhood
            {
                for (int i = -1; i <= 1; i++) 
                {
                    SDL_GetRGB(copy[(y + j) * pitch + (x + i)], surface -> format, &r, &g, &b);
                    gray[k] = r;   // r=g=b since grayscale
                    k++;
                }
            }

            for (int m = 0; m < 8; m++)         // simple bubble sort
            {
                for (int n = m + 1; n < 9; n++) 
                {
                    if (gray[n] < gray[m]) 
                    {
                        Uint8 tmp = gray[m];
                        gray[m] = gray[n];
                        gray[n] = tmp;
                    }
                }
            }

            pixels[y * pitch + x] = SDL_MapRGB(surface -> format, gray[4], gray[4], gray[4]);   // replace with median
        }
    }
    
    free(copy);
}