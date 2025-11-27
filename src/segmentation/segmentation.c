#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "segmentation.h"

typedef struct {
    int x, y, w, h;         // bounding box, (x,y) is top-left corner
    SDL_Surface* surface;   // cropped letter surface
} LetterBox;

void draw_rectangle_on_surface(SDL_Surface* surface, LetterBox box, Uint8 r, Uint8 g, Uint8 b, int thickness, int expand)
{
    if (!surface || box.w <= 0 || box.h <= 0)
        return;

    Uint32 color = SDL_MapRGB(surface->format, r, g, b);
    Uint32* pixels = (Uint32*)surface->pixels;
    int pitch = surface->pitch / 4;

    int x1 = box.x - expand;
    int y1 = box.y - expand;
    int x2 = box.x + box.w + expand - 1;
    int y2 = box.y + box.h + expand - 1;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= surface->w) x2 = surface->w - 1;
    if (y2 >= surface->h) y2 = surface->h - 1;

    for (int t = 0; t < thickness; t++)
    {
        int tx1 = x1 - t; if (tx1 < 0) tx1 = 0;
        int ty1 = y1 - t; if (ty1 < 0) ty1 = 0;
        int tx2 = x2 + t; if (tx2 >= surface->w) tx2 = surface->w - 1;
        int ty2 = y2 + t; if (ty2 >= surface->h) ty2 = surface->h - 1;

        for (int x = tx1; x <= tx2; x++) pixels[ty1 * pitch + x] = color;
        for (int x = tx1; x <= tx2; x++) pixels[ty2 * pitch + x] = color;
        for (int y = ty1; y <= ty2; y++) pixels[y * pitch + tx1] = color;
        for (int y = ty1; y <= ty2; y++) pixels[y * pitch + tx2] = color;
    }
}

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

        if (r == 255 && g == 255 && b == 255) continue; // skips white pixel

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

void clear_surface(SDL_Surface* surface, int x, int y, int w, int h)
{
    Uint32* pixels = (Uint32*)surface -> pixels;
    int pitch = surface -> pitch / 4;

    for(int i = x; i < x + w; i++)
    {
        for(int j = y; j < y + h; j++)
        {
            Uint8 r, g, b, a;
            int index = j * pitch + i;
            SDL_GetRGBA(pixels[index], surface -> format, &r, &g, &b, &a);

            if(r == 255 && g == 0 && b == 0)
            {
                pixels[index] = SDL_MapRGBA(surface -> format, 255, 255, 255, 255);
            }
        }
    }
}

LetterBox* extract_letters(SDL_Surface* surface, int* out_count, Uint8 color_r, Uint8 color_g, Uint8 color_b)
{
    int capacity = 64;          // starting capacity, will realloc if needed
    int count = 0;              // tracker to reallocate

    LetterBox* boxes = malloc(capacity * sizeof(LetterBox));                // dynamic array of letter boxes
    Uint32 visited_color = SDL_MapRGBA(surface -> format, color_r, color_g, color_b, 255);    // arbitrary visited color (red)

    int w = surface -> w;       // pixels per row
    int h = surface -> h;       // n of rows

    int min_w = 2;              // min w threshold
    int min_h = 13;              // min h threshold

    int max_w = 60;             // max w threshold
    int max_h = 60;             // max h threshold

    Uint32* pixels = (Uint32*)surface -> pixels;    // raw pixels
    int pitch = surface -> pitch / 4;               // n of bytes per row / 4 bc theres 4 bytes per pixel (RGBA8888 format)

    for(int y = 0; y < h; y++) 
    {
        for(int x = 0; x < w; x++) 
        {
            int index = y * pitch + x;      // index in the pixel array
            Uint8 r, g, b, a;

            SDL_GetRGBA(pixels[index], surface -> format, &r, &g, &b, &a);

            if(!(r == 255 && g == 255 && b == 255) && pixels[index] != visited_color)  // not white nor visited
            {
                if(count >= capacity)       // realloc if needed
                {
                    capacity *= 2;
                    boxes = realloc(boxes, capacity * sizeof(LetterBox));
                }

                SDL_Rect box;                                     // creates rectangle for bounding box
                flood_fill(surface, x, y, visited_color, &box);   // flood fill to find bounding box

                if(box.w < min_w || box.h < min_h || box.w > max_w || box.h > max_h)    // ignore small noises and big boxes like grid or lines
                {
                    clear_surface(surface, box.x, box.y, box.w, box.h);
                    continue;
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

    int centerA = A -> y + A -> h / 2;
    int centerB = B -> y + B -> h / 2;

    if (abs(centerA - centerB) > 5) // delta y
    {
        return A -> y - B -> y; // top to bottom
    }
    else if (abs(A -> h - B -> h) > 8) // delta h
    {
        return A -> y - B -> y; // top to bottom
    }
    else 
    {
        return A -> x - B -> x; // left to right
    }
}

int compute_median_dx(LetterBox* letters, int n)
{
    if (n <= 1)
        return 0;

    int gaps[n - 1];
    int count = 0;

    for (int i = 1; i < n; i++)
    {
        int dx = letters[i].x - letters[i - 1].x;
        if (dx > 0)
            gaps[count++] = dx;
    }

    if (count == 0)
        return 0;

    for (int i = 1; i < count; i++)
    {
        int key = gaps[i];
        int j = i - 1;
        while (j >= 0 && gaps[j] > key)
        {
            gaps[j + 1] = gaps[j];
            j--;
        }
        gaps[j + 1] = key;
    }

    if (count % 2 == 1)
        return gaps[count / 2];
    else
        return (gaps[count/2 - 1] + gaps[count/2]) / 2;
}

void extract_boxes(SDL_Surface* surface, LetterBox* letters, int n_letters, LetterBox** out_boxes, int* out_count, int* out_max)
{
    LetterBox* boxes = malloc(n_letters * sizeof(LetterBox));
    int count = 0;
    int max_w = 0;
    int median_dx = compute_median_dx(letters, n_letters);

    int x = 0, y = 0, w = 0, h = 0;

    for (int i = 0; i < n_letters; i++) 
    {
        int letter_x = letters[i].x;
        int letter_y = letters[i].y;
        int letter_w = letters[i].w;
        int letter_h = letters[i].h;

        int end_x = x + w;
        int dx = abs(letter_x - end_x);

        if (i == 0) 
        {
            x = letter_x;
            y = letter_y;
            w = letter_w;
            h = letter_h;
            continue;
        }

        if (i == n_letters - 1 || abs(letter_y - y) > 10 || dx > median_dx)
        {
            if(i == n_letters - 1)
            {
                w = w + letters[i].w + (letters[i].x - (x + w)); 
            
                if(letters[i].h > h)    h = letters[i].h;
                if(letters[i].y < y)    y = letters[i].y;
            }

            boxes[count].x = x;
            boxes[count].y = y;
            boxes[count].w = w;
            boxes[count].h = h;

            SDL_Rect rect = { x, y, w, h };

            boxes[count].surface = crop_surface(surface, rect);
            count++;

            if(w > max_w)   max_w = w;

            x = letter_x;
            y = letter_y;
            w = letter_w;
            h = letter_h;
        }
        else
        {
            w = w + letters[i].w + (letters[i].x - (x + w)); 
            
            int top = (letters[i].y < y) ? letters[i].y : y;
            int bottom = ( (y + h) > (letters[i].y + letters[i].h) ) ? (y + h) : (letters[i].y + letters[i].h);
    
            y = top;
            h = bottom - top;
        }
    }

    printf("Detected %d boxes\n", count);

    *out_boxes = boxes;
    *out_count = count;
    *out_max = max_w;
}

LetterBox build_grid(SDL_Surface* surface, LetterBox* boxes, int box_count, int max_w, int* out_count, int* out_garbage)
{
    int x = 0, y = 0, h = 0;

    int words_count = 0;
    int garbage_count = 0;
    
    for(int i = 0; i < box_count; i++)
    {
        if(abs(max_w - boxes[i].w) < 20)
        {
            if(x == 0)
            {
                x = boxes[i].x;
                y = boxes[i].y;
                h += boxes[i].h;
            }
            else if(abs(x - boxes[i].x) < 10)
            {
                h = h + boxes[i].h + (boxes[i].y - (y + h));

                if(boxes[i].x < x)
                {
                    x = boxes[i].x;
                }
            }
        }
        else
        {
            if(x == 0 && boxes[i].y < 100 && boxes[i].w < 80)
            {
                garbage_count++;
            }

            words_count++;
        }
    }

    LetterBox grid;
    grid.x = x;
    grid.y = y;
    grid.w = max_w;
    grid.h = h;

    SDL_Rect rect = { x, y, max_w, h };

    grid.surface = crop_surface(surface, rect);

    *out_count = words_count;
    *out_garbage = garbage_count;

    return grid;
}

void save_letters(SDL_Surface* surface, SDL_Surface* display, char* file) {

    int n_letters;      // number of detected letters

    LetterBox* letters = extract_letters(surface, &n_letters, 255, 0, 0);
    qsort(letters, n_letters, sizeof(LetterBox), compare_letters);

    LetterBox* boxes;
    int box_count;
    int max_box_w;

    extract_boxes(surface, letters, n_letters, &boxes, &box_count, &max_box_w);

    int words_count;
    int garbage_count;

    LetterBox grid = build_grid(surface, boxes, box_count, max_box_w, &words_count, &garbage_count);

    draw_rectangle_on_surface(display, grid, 255, 0, 0, 4, 10);

    printf("Detected %d garbages\n", garbage_count);
    printf("Detected %d words\n", words_count - garbage_count);
    LetterBox* words = malloc((words_count - garbage_count)* sizeof(LetterBox));

    int words_index = 0;

    for(int i = 0; i < box_count; i++) 
    {
        if(abs(max_box_w - boxes[i].w) > 20)
        {
            if(words_index >= garbage_count)
            {
                words[words_index - garbage_count] = boxes[i];
                draw_rectangle_on_surface(display, boxes[i], 0, 0, 255, 2, 5);

                if(words_index == words_count)  break;
            }

            words_index++;
        }
    }

    int n_grid_letters;

    LetterBox* grid_letters = extract_letters(grid.surface, &n_grid_letters, 0, 0, 255);
    qsort(grid_letters, n_grid_letters, sizeof(LetterBox), compare_letters);

    printf("Detected %d letters from the grid\n", n_grid_letters);

    const char* dot = strrchr(file, '.');
    const char* slash = strrchr(file, '/');
    const char* start = (slash) ? slash + 1 : file;

    int row = 0;
    int col = 0;

    int prev_y = 0;

    for(int i = 0; i < n_grid_letters; i++)
    {
        if(abs(grid_letters[i].y - prev_y) > 10)
        {
            row++;
            col = 0;
        }

        char filename[128];

        snprintf(filename, sizeof(filename), "datasets/%.*s/grid_letters/letter[%d,%d].bmp", (int)(dot - start), start, row, col);

        if(SDL_SaveBMP(grid_letters[i].surface, filename) != 0) 
        {
            printf("Failed to save %s: %s\n", filename, SDL_GetError());
        }

        col++;

        prev_y = grid_letters[i].y;
    }

    for(int i = 0; i < n_grid_letters; i++) 
    {
        SDL_FreeSurface(grid_letters[i].surface);
    }

    free(grid_letters);

    words_count -= garbage_count;

    for(int i = 0; i < words_count; i++)
    {
        int n_word_letter;

        LetterBox* word_letters = extract_letters(words[i].surface, &n_word_letter, 0, 0, 255);
        qsort(word_letters, n_word_letter, sizeof(LetterBox), compare_letters);

        for(int j = 0; j < n_word_letter; j++)
        {
            char filename[128];

            printf("Word %d, letter %d: x=%d, y=%d, w=%d, h=%d\n", i + 1, j + 1, word_letters[j].x, word_letters[j].y, word_letters[j].w, word_letters[j].h);

            snprintf(filename, sizeof(filename), "datasets/%.*s/words_letters/word[%d,%d].bmp", (int)(dot - start), start, i, j);

            if(SDL_SaveBMP(word_letters[j].surface, filename) != 0) 
            {
                printf("Failed to save %s: %s\n", filename, SDL_GetError());
            }
        }

        for(int j = 0; j < n_word_letter; j++) 
        {
            SDL_FreeSurface(word_letters[j].surface);
        }

        free(word_letters);
    }

    free(words);

    for(int i = 0; i < box_count; i++) 
    {
        SDL_FreeSurface(boxes[i].surface);
    }

    free(boxes);

    for(int i = 0; i < n_letters; i++) 
    {
        SDL_FreeSurface(letters[i].surface);
    }

    free(letters);
}




