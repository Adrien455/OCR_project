#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "rotate.h"

SDL_Surface* rotozoomSurface(SDL_Surface* surface, double angle_deg)
{
    double a = angle_deg * M_PI / 180.0;
    double s = sin(a), c = cos(a);

    int sw = surface -> w;
    int sh = surface -> h;

    int dw = (int)ceil((fabs(sw * c) + fabs(sh * s)));
    int dh = (int)ceil((fabs(sw * s) + fabs(sh * c)));

    SDL_Surface* rotated = SDL_CreateRGBSurfaceWithFormat(0, dw, dh, surface -> format -> BitsPerPixel, surface -> format -> format);

    Uint32 white = SDL_MapRGBA(rotated -> format, 255, 255, 255, 255);
    SDL_FillRect(rotated, NULL, white);

    double scx = (sw - 1) * 0.5;
    double scy = (sh - 1) * 0.5;
    double dcx = (dw - 1) * 0.5;
    double dcy = (dh - 1) * 0.5;

    Uint32* sp = (Uint32*)surface -> pixels;
    Uint32* dp = (Uint32*)rotated -> pixels;

    int sstride = surface -> pitch / 4;
    int dstride = rotated -> pitch / 4;

    for (int y = 0; y < dh; ++y) 
    {
        double dy = y - dcy;

        for (int x = 0; x < dw; ++x) 
        {
            double dx = x - dcx;

            double sx =  c * dx + s * dy + scx + 0.5;
            double sy = -s * dx + c * dy + scy + 0.5;

            int ix = (int)floor(sx);
            int iy = (int)floor(sy);

            Uint32 color = white;

            if ((unsigned)ix < (unsigned)sw && (unsigned)iy < (unsigned)sh) 
            {
                color = sp[iy * sstride + ix];
            }

            dp[y * dstride + x] = color;
        }
    }

    return rotated;
}

double compute_skew_angle(SDL_Surface* surface)
{
    int w = surface -> w;
    int h = surface -> h;

    Uint32* pixels = (Uint32*)surface -> pixels;
    int pitch = surface -> pitch / 4;

    double best_angle = 0.0;
    double best_score = -1.0;

    for (double a = -30.0; a <= 30.0; a += 0.5) 
    {
        double s = sin(a * M_PI / 180.0);
        double c = cos(a * M_PI / 180.0);
        int proj[1500] = {0};       // 600 * 800 dims
        int bias = w / 2;

        for (int y = 0; y < h; y += 2) 
        {
            const Uint32* row = pixels + y * pitch;

            for (int x = 0; x < w; x += 2) 
            {
                Uint8 r = (row[x] >> 24) & 0xFF;

                if (r < 128) 
                {
                    int yr = (int)(x * s + y * c) + bias;

                    if ((unsigned)yr < 1500U) 
                    {
                        proj[yr]++;
                    }
                }
            }
        }

        double score = 0.0;

        for (int i = 0; i < 1500; ++i)
        {
            score += (double)proj[i] * (double)proj[i];
        }

        if (score > best_score) 
        {
            best_score = score;
            best_angle = a;
        }
    }

    return best_angle;
}

       

SDL_Surface* rotate(SDL_Surface* surface) 
{
    double angle = compute_skew_angle(surface);

    if (fabs(angle) < 0.2) 
    {
        printf("Rotation not needed\n");
        return surface; // No rotation needed
    }

    printf("Detected skew angle: %.2f degrees\n", angle);

    SDL_Surface *rotated = rotozoomSurface(surface, angle);

    return rotated;
}
