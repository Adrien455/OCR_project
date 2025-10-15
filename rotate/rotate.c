#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "rotate.h"

SDL_Surface *rotozoomSurface(SDL_Surface* src, double angle_deg, double zoom)
{
    double angle = angle_deg * M_PI / 180.0;
    double s = sin(angle);
    double c = cos(angle);

    int src_w = src->w;
    int src_h = src->h;
    int dst_w = (int)ceil((fabs(src_w * c) + fabs(src_h * s)) * zoom);
    int dst_h = (int)ceil((fabs(src_w * s) + fabs(src_h * c)) * zoom);

    SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(
        0, dst_w, dst_h,
        src -> format -> BitsPerPixel,
        src -> format -> format
    );

    int src_cx = src_w / 2;
    int src_cy = src_h / 2;
    int dst_cx = dst_w / 2;
    int dst_cy = dst_h / 2;

    Uint32* src_pixels = (Uint32*)src -> pixels;
    Uint32* dst_pixels = (Uint32*)dst -> pixels;
    int src_pitch = src -> pitch / 4;
    int dst_pitch = dst -> pitch / 4;

    for (int y = 0; y < dst_h; ++y)
    {
        for (int x = 0; x < dst_w; ++x)
        {
            // map dst -> src coordinates (inverse rotation)
            double dx = (x - dst_cx) / zoom;
            double dy = (y - dst_cy) / zoom;

            int src_x = (int)( c * dx + s * dy + src_cx );
            int src_y = (int)(-s * dx + c * dy + src_cy );

            Uint32 color;
            if (src_x >= 0 && src_x < src_w && src_y >= 0 && src_y < src_h)
                color = src_pixels[src_y * src_pitch + src_x];
            else
                color = SDL_MapRGB(dst -> format, 255, 255, 255);  // white background

            dst_pixels[y * dst_pitch + x] = color;
        }
    }

    return dst;
}

static double projection_score(SDL_Surface* surface) 
{
    Uint32* pixels = (Uint32*)surface -> pixels;
    int w = surface -> w;
    int h = surface -> h;
    int pitch = surface -> pitch / 4; // pixels per row in Uint32
    double score = 0.0;

    Uint32 black_pixel = SDL_MapRGBA(surface -> format, 0, 0, 0, 255);

    for (int y = 0; y < h; y++) 
    {
        int row_sum = 0;
        Uint32* row = pixels + y * pitch;
        for (int x = 0; x < w; x++) 
        {
            if (row[x] == black_pixel)
            { 
                row_sum++;
            }
        }
        score += (double)(row_sum * row_sum); // squared sum favors aligned rows
    }

    return score;
}

// Robust skew detection using coarse + fine search
double find_skew_angle(SDL_Surface* surface) 
{
    double best_angle = 0.0;
    double best_score = -DBL_MAX;

    // --- Coarse search: -45° to 45° in 2° steps ---
    for (double angle = -45.0; angle <= 45.0; angle += 2.0) 
    {
        SDL_Surface* rotated = rotozoomSurface(surface, angle, 1.0);
        double score = projection_score(rotated);
        SDL_FreeSurface(rotated);

        if (score > best_score) 
        {
            best_score = score;
            best_angle = angle;
        }
    }

    // --- Fine search around coarse best ±2° in 0.2° steps ---
    double fine_best = best_angle;
    double fine_score = best_score;

    for (double angle = best_angle - 2.0; angle <= best_angle + 2.0; angle += 0.2) 
    {
        SDL_Surface* rotated = rotozoomSurface(surface, angle, 1.0);
        double score = projection_score(rotated);
        SDL_FreeSurface(rotated);

        if (score > fine_score) 
        {
            fine_score = score;
            fine_best = angle;
        }
    }

    return -fine_best;
}

// Main rotate function: returns a square surface containing the rotated image
SDL_Surface* rotate(SDL_Surface* surface) 
{
    double angle = find_skew_angle(surface);
    printf("%f\n", angle);

    // Skip rotation if angle is tiny
    if (fabs(angle) < 0.2) 
    {
        return SDL_ConvertSurface(surface, surface -> format, 0); // copy as is
    }

    // Rotate image
    SDL_Surface *rotated = rotozoomSurface(surface, -angle, 1.0);

    // Compute square size to fully contain rotated image
    int w = rotated -> w;
    int h = rotated -> h;
    int square_size = (int)ceil(sqrt(w * w + h * h));

    double scale = (double)square_size / (double) (w > h ? w : h); // scale to fit

    SDL_Surface* zoomed = rotozoomSurface(rotated, 0.0, scale); // scale only, no rotation

    SDL_Surface* square = SDL_CreateRGBSurface(0, square_size, square_size,
        rotated -> format -> BitsPerPixel,
        rotated -> format -> Rmask,
        rotated -> format -> Gmask,
        rotated -> format -> Bmask,
        rotated -> format -> Amask
    );

    // Fill with white background
    SDL_FillRect(square, NULL, SDL_MapRGB(square->format, 255, 255, 255));

    // Center rotated image inside the square
    SDL_Rect dst = { (square -> w - zoomed -> w)/2, (square -> h - zoomed -> h)/2, zoomed -> w, zoomed -> h };
    SDL_BlitSurface(zoomed, NULL, square, &dst);

    SDL_FreeSurface(rotated);
    SDL_FreeSurface(zoomed);
    return square;
}
