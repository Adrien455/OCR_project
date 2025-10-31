#ifndef LOADER_H
#define LOADER_H

void initialize(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture, char *file, SDL_Surface **surface);
void terminate(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture);
void save_bmp(SDL_Renderer *renderer);
int event_handler();

#endif
