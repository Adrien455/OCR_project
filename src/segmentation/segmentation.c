#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "segmentation.h"

typedef struct {
    int x, y, w, h;         // bounding box
    int row, col;           // grid position
    SDL_Surface* surface;   // cropped letter surface
    int is_grid;            // for now always 1 (part of the grid)
} LetterBox;

void flood_fill(SDL_Surface* surface, int start_x, int start_y, Uint32 visited_color, SDL_Rect* bbox)
{
    int w = surface -> w, h = surface -> h;
    Uint32* pixels = (Uint32*)surface -> pixels;
    int pitch = surface -> pitch / 4;

    int min_x = start_x, max_x = start_x;
    int min_y = start_y, max_y = start_y;

    typedef struct { int x, y; } Point;

    Point* stack = malloc(w * h * sizeof(Point));
    int stack_size = 0;
    stack[stack_size++] = (Point){start_x, start_y};

    while(stack_size > 0) 
    {
        Point p = stack[--stack_size];
        int idx = p.y * pitch + p.x;
        if (pixels[idx] == visited_color) continue;

        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[idx], surface -> format, &r, &g, &b, &a);
        if ((r != 0 || g != 0 || b != 0) && a != 0) continue; // white pixel

        pixels[idx] = visited_color; // mark visited

        if(p.x < min_x) min_x = p.x;
        if(p.x > max_x) max_x = p.x;
        if(p.y < min_y) min_y = p.y;
        if(p.y > max_y) max_y = p.y;

        if(p.x > 0) stack[stack_size++] = (Point){p.x - 1, p.y};
        if(p.x < w - 1) stack[stack_size++] = (Point){p.x + 1, p.y};
        if(p.y > 0) stack[stack_size++] = (Point){p.x, p.y - 1};
        if(p.y < h - 1) stack[stack_size++] = (Point){p.x, p.y + 1};
    }

    bbox -> x = min_x;
    bbox -> y = min_y;
    bbox -> w = max_x - min_x + 1;
    bbox -> h = max_y - min_y + 1;

    free(stack);
}

SDL_Surface* crop_surface(SDL_Surface* src, SDL_Rect bbox)
{
    SDL_Surface* dst = SDL_CreateRGBSurface(0, bbox.w, bbox.h,
        src -> format->BitsPerPixel,
        src -> format->Rmask,
        src -> format->Gmask,
        src -> format->Bmask,
        src -> format->Amask
    );

    SDL_BlitSurface(src, (SDL_Rect*)&bbox, dst, NULL);
    return dst;
}

// Sorting helpers for grid assignment
int compare_y(const void* a, const void* b)
{ 
    return ((LetterBox*)a) -> y - ((LetterBox*)b) -> y; 
}

int compare_x(const void* a, const void* b)
{ 
    return ((LetterBox*)a) -> x - ((LetterBox*)b) -> x; 
}

// Assign row/column positions based on vertical/horizontal gaps
void assign_grid_positions(LetterBox* boxes, int n, int vertical_thresh, int horizontal_thresh)
{
    if(n==0) return;

    qsort(boxes, n, sizeof(LetterBox), compare_y);

    int row_index = 0;
    int i = 0;

    while(i < n) 
    {
        int row_start = i;
        int row_end = i;

        for(int j=i+1; j<n; j++) 
        {
            if(boxes[j].y - boxes[row_start].y > vertical_thresh) break;
            row_end = j;
        }

        qsort(&boxes[row_start], row_end-row_start+1, sizeof(LetterBox), compare_x);

        int col_index = 0;
        int last_x = boxes[row_start].x;

        for(int k=row_start; k<=row_end; k++) 
        {
            if(k != row_start && boxes[k].x - last_x > horizontal_thresh) col_index++;
            boxes[k].row = row_index;
            boxes[k].col = col_index;
            last_x = boxes[k].x;
        }

        row_index++;
        i = row_end+1;
    }
}

// Full pipeline: flood-fill, crop, assign grid positions, mark pre-filled
LetterBox* extract_letters(SDL_Surface* surface, int* out_count)
{
    int capacity = 64;
    int count = 0;

    LetterBox* boxes = malloc(capacity * sizeof(LetterBox));
    Uint32 visited_color = SDL_MapRGBA(surface->format, 255, 0, 0, 255);

    int w = surface -> w, h = surface -> h;
    Uint32* pixels = (Uint32*)surface -> pixels;
    int pitch = surface -> pitch / 4;

    for(int y = 0; y < h; y++) 
    {
        for(int x = 0; x < w; x++) 
        {
            int idx = y * pitch + x;
            Uint8 r, g, b, a;

            SDL_GetRGBA(pixels[idx], surface -> format, &r, &g, &b, &a);

            if(r == 0 && g == 0 && b == 0 && a != 0 && pixels[idx] != visited_color)
            {
                if(count >= capacity) 
                {
                    capacity *= 2;
                    boxes = realloc(boxes, capacity * sizeof(LetterBox));
                }

                SDL_Rect bbox_rect;
                flood_fill(surface, x, y, visited_color, &bbox_rect);

                boxes[count].x = bbox_rect.x;
                boxes[count].y = bbox_rect.y;
                boxes[count].w = bbox_rect.w;
                boxes[count].h = bbox_rect.h;
                boxes[count].surface = crop_surface(surface, bbox_rect);
                boxes[count].is_grid = 1; // for now always part of the grid

                count++;
            }
        }
    }

    // Assign row/col positions
    int avg_h = 0, avg_w = 0;

    for(int i = 0; i < count; i++) 
    { 
        avg_h += boxes[i].h; 
        avg_w += boxes[i].w; 
    }

    avg_h = (count > 0) ? avg_h / count : 10;
    avg_w = (count > 0) ? avg_w / count : 10;

    assign_grid_positions(boxes, count, avg_h / 2, avg_w / 2);

    *out_count = count;
    return boxes;
}

void save_letters(SDL_Surface* surface, char* file) {

    int n_letters; // number of detected letters

    LetterBox* letters = extract_letters(surface, &n_letters);

    printf("Detected %d letters\n", n_letters);

     const char* dot = strrchr(file, '.');
     const char* slash = strrchr(file, '/');
     const char* start = (slash) ? slash + 1 : file;

    // Loop through each letter and save as BMP
    for(int i = 0; i < n_letters; i++) 
    {
        char filename[128];
        // Create a filename using row and col to preserve grid order
        snprintf(filename, sizeof(filename), "datasets/%.*s/letter_row[%d]_col[%d].bmp", (int)(dot - start), start, letters[i].row, letters[i].col);

        // Save the cropped SDL_Surface
        if(SDL_SaveBMP(letters[i].surface, filename) != 0) 
        {
            printf("Failed to save %s: %s\n", filename, SDL_GetError());
        }
    }

    // Free surfaces and array
    for(int i = 0; i < n_letters; i++) 
    {
        SDL_FreeSurface(letters[i].surface);
    }

    free(letters);
}




