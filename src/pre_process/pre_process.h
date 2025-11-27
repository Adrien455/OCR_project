#ifndef PRE_PROCESS_H
#define PRE_PROCESS_H

void to_gray_scale(SDL_Surface *surface, int *hist_ptr);
void binarize(SDL_Surface *surface, int *hist);
void denoise(SDL_Surface *surface);

#endif
