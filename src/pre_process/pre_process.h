#ifndef PRE_PROCESS_H
#define PRE_PROCESS_H

void to_gray_scale(SDL_Surface *surface);
void binarize(SDL_Surface *surface);
void increaseContrast(SDL_Surface *surface);
void denoise(SDL_Surface *surface);

#endif
