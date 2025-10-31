#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "segmentation.h"

typedef struct {
    int x, y, w, h;         // bounding box, (x,y) is top-left corner
    SDL_Surface* surface;   // cropped letter surface
} LetterBox;

void flood_fill(SDL_Surface* surface, int start_x, int start_y, Uint32 visited_color, SDL_Rect* bbox)
{
    int w = surface -> w;
    int h = surface -> h;

    Uint32* pixels = (Uint32*)surface -> pixels;
    int pitch = surface -> pitch / 4;

    int min_x = start_x, max_x = start_x;   // initialize bounding box coordinates
    int min_y = start_y, max_y = start_y;

    typedef struct { int x, y; } Pixel;     // represents pixel coordinates

    Pixel* stack = malloc(w * h * sizeof(Pixel));       // w * h not really necessary but safe
    int stack_size = 0;
    stack[stack_size++] = (Pixel){start_x, start_y};    // push starting point and increment

    while(stack_size > 0) 
    {
        Pixel p = stack[--stack_size];  // decrement and pop

        int index = p.y * pitch + p.x;
        
        if (pixels[index] == visited_color) continue;   // already visited

        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[index], surface -> format, &r, &g, &b, &a);

        if ((r != 0 || g != 0 || b != 0) && a != 0) continue; // white pixel since its only black and white

        pixels[index] = visited_color;  // mark visited

        if(p.x < min_x) min_x = p.x;    // update bounding box coordinates
        if(p.x > max_x) max_x = p.x;
        if(p.y < min_y) min_y = p.y;
        if(p.y > max_y) max_y = p.y;

        if(p.x > 0) stack[stack_size++] = (Pixel){p.x - 1, p.y};        // add left neighbor
        if(p.x < w - 1) stack[stack_size++] = (Pixel){p.x + 1, p.y};    // add right neighbor
        if(p.y > 0) stack[stack_size++] = (Pixel){p.x, p.y - 1};        // add top neighbor
        if(p.y < h - 1) stack[stack_size++] = (Pixel){p.x, p.y + 1};    // add bottom neighbor
    }

    bbox -> x = min_x;
    bbox -> y = min_y;
    bbox -> w = max_x - min_x + 1;   // +1 to include the pixel at max_x and max_y
    bbox -> h = max_y - min_y + 1;

    free(stack);
}

SDL_Surface* crop_surface(SDL_Surface* surface, SDL_Rect bbox)
{
    SDL_Surface* cropped = SDL_CreateRGBSurfaceWithFormat(0, bbox.w, bbox.h, surface -> format -> BitsPerPixel, surface -> format -> format);
    SDL_BlitSurface(surface, &bbox, cropped, NULL);     // copy bbox from src to dst

    return cropped;
}

LetterBox* extract_letters(SDL_Surface* surface, int* out_count, LetterBox* grid_box)
{
    int capacity = 64;          // starting capacity, will realloc if needed
    int count = 0;              // tracker to reallocate

    LetterBox* boxes = malloc(capacity * sizeof(LetterBox));                // dynamic array of letter boxes
    Uint32 visited_color = SDL_MapRGBA(surface -> format, 255, 0, 0, 255);    // arbitrary visited color (red)

    int w = surface -> w;       // pixels per row
    int h = surface -> h;       // n of rows

    int max_size = 0;        // max size of letters (basically the grid)

    Uint32* pixels = (Uint32*)surface -> pixels;    // raw pixels
    int pitch = surface -> pitch / 4;               // n of bytes per row / 4 bc theres 4 bytes per pixel (RGBA8888 format)

    for(int y = 0; y < h; y++) 
    {
        for(int x = 0; x < w; x++) 
        {
            int index = y * pitch + x;      // index in the pixel array
            Uint8 r, g, b, a;

            SDL_GetRGBA(pixels[index], surface -> format, &r, &g, &b, &a);

            if(r == 0 && g == 0 && b == 0 && a != 0 && pixels[index] != visited_color)  // black pixel and not visited
            {
                if(count >= capacity)       // realloc if needed
                {
                    capacity *= 2;
                    boxes = realloc(boxes, capacity * sizeof(LetterBox));
                }

                SDL_Rect box;                                     // creates rectangle for bounding box
                flood_fill(surface, x, y, visited_color, &box);   // flood fill to find bounding box

                if(box.w < 5 && box.h < 5) continue; // ignore small noise

                if(box.w * box.h > max_size)    // update grid bounding box
                {
                    max_size = box.w * box.h;
                    grid_box -> x = box.x;
                    grid_box -> y = box.y;
                    grid_box -> w = box.w;
                    grid_box -> h = box.h;
                }

                boxes[count].x = box.x;       // store bounding box
                boxes[count].y = box.y;
                boxes[count].w = box.w;
                boxes[count].h = box.h;
                boxes[count].surface = crop_surface(surface, box);

                count++;
            }
        }
    }

    *out_count = count;
    return boxes;
}

int compare_letters(const void* a, const void* b)   // VERY IMPORTANT 
{
    const LetterBox* A = a;
    const LetterBox* B = b;

    if (abs(A -> y - B -> y) > 4) // delta y
    {
        return A -> y - B -> y; // top to bottom
    }
    else if (abs(A -> h - B -> h) > 3) // delta h
    {
        return A -> y - B -> y; // top to bottom
    }
    else 
    {
        return A -> x - B -> x; // left to right
    }
    
}

int is_part_of_grid(LetterBox grid_box, LetterBox letter_box) 
{
    int grid_right = grid_box.x + grid_box.w;
    int grid_bottom = grid_box.y + grid_box.h;

    int letter_right = letter_box.x + letter_box.w;
    int letter_bottom = letter_box.y + letter_box.h;

    if (letter_box.x >= grid_box.x && letter_right <= grid_right && letter_box.y >= grid_box.y && letter_bottom <= grid_bottom) 
    {
        return 1; // is part of the grid
    }

    return 0; // not part of the grid
}

void save_letters(SDL_Surface* surface, char* file) {

    int n_letters;      // number of detected letters

    LetterBox grid_box; // bounding box of the grid

    LetterBox* letters = extract_letters(surface, &n_letters, &grid_box);
    qsort(letters, n_letters, sizeof(LetterBox), compare_letters);

    printf("Detected %d letters\n", n_letters - 1);

    const char* dot = strrchr(file, '.');
    const char* slash = strrchr(file, '/');
    const char* start = (slash) ? slash + 1 : file;

    int count_in_grid = 0;
    int count_outside_grid = 0;

    int count_words = 0;

    int prev_x = 0;
    int prev_y = 0;

    for(int i = 0; i < n_letters; i++)
    {
        char filename[128];

        if(letters[i].w == grid_box.w && letters[i].h == grid_box.h) 
        {
            printf("Grid: x=%d, y=%d, w=%d, h=%d\n", letters[i].x, letters[i].y, letters[i].w, letters[i].h);
            snprintf(filename, sizeof(filename), "datasets/%.*s/grid.bmp", (int)(dot - start), start);

            if(SDL_SaveBMP(letters[i].surface, filename) != 0) 
            {
                printf("Failed to save %s: %s\n", filename, SDL_GetError());
            }

            continue;
        }

        if(is_part_of_grid(grid_box, letters[i])) 
        {
            printf("Letter %d: x=%d, y=%d, w=%d, h=%d\n", count_in_grid, letters[i].x, letters[i].y, letters[i].w, letters[i].h);
            count_in_grid++;

            snprintf(filename, sizeof(filename), "datasets/%.*s/grid_letters/letter[%d].bmp", (int)(dot - start), start, count_in_grid);

            if(SDL_SaveBMP(letters[i].surface, filename) != 0) 
            {
                printf("Failed to save %s: %s\n", filename, SDL_GetError());
            }

            count_outside_grid = 0;
        }
        else 
        {
            if(count_outside_grid == 0) 
            {
                count_words++;
            }
            else if(abs(letters[i].x - prev_x) > 50 || abs(letters[i].y - prev_y) > 10)     // word on same line or very small words
            {
                count_words++;
                count_outside_grid = 0;
            }

            printf("Word[%d], Letter %d: x=%d, y=%d, w=%d, h=%d\n", count_words, count_outside_grid, letters[i].x, letters[i].y, letters[i].w, letters[i].h);
            count_outside_grid++;

            snprintf(filename, sizeof(filename), "datasets/%.*s/words_letters/word[%d]_letter[%d].bmp", (int)(dot - start), start, count_words, count_outside_grid);

            if(SDL_SaveBMP(letters[i].surface, filename) != 0) 
            {
                printf("Failed to save %s: %s\n", filename, SDL_GetError());
            }

            prev_x = letters[i].x;
            prev_y = letters[i].y;
        }
    }

    for(int i = 0; i < n_letters; i++) 
    {
        SDL_FreeSurface(letters[i].surface);
    }

    free(letters);
}




