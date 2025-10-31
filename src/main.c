// libraries
#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>

// headers
#include "loader/loader.h"
#include "event_handler/event_handler.h"
#include "neuronal_network/mlp.h"
#include "solver/solver.h"

int run_mlp()
{
    double X[4][2] = { {0,0}, {0,1}, {1,0}, {1,1} };
    double Y[4] = { 0, 0, 1, 1 };
    int H = 3;
    MLP *mlp = mlp_create(2, H);

    for (int epoch = 0; epoch < 5000; ++epoch)
    {
        double loss = 0.0;
        for (int i = 0; i < 4; ++i)
        {
            double h[H];
            double y = mlp_forward(mlp, X[i], h);
            loss += -(Y[i] * log(y + 1e-8) + (1.0 - Y[i]) * log(1.0 - y + 1e-8));
            mlp_backward(mlp, X[i], h, y, Y[i], 0.1);
        }
        if (epoch % 500 == 0)
            printf("epoch=%d loss=%.6f\n", epoch, loss);
    }

    for (int i = 0; i < 4; ++i)
    {
        double h[H];
        double y = mlp_forward(mlp, X[i], h);
        printf("A=%.0f B=%.0f -> y=%.3f (target=%.0f)\n", X[i][0], X[i][1], y, Y[i]);
    }

    mlp_free(mlp);
    return 0;
}

int run_solver(const char *filename, char *word)
{
    for (size_t i = 0; i < strlen(word); i++)
    {
        if (word[i] >= 'a' && word[i] <= 'z')
            word[i] -= 32;
        if (!(word[i] >= 'A' && word[i] <= 'Z'))
        {
            fprintf(stderr, "Error: Your word is weird\n");
            return 1;
        }
    }

    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file");
        return 1;
    }

    char line[256];
    char grid[100][100];
    int rows = 0;
    int cols = 0;

    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = '\0';
        strcpy(grid[rows], line);
        rows++;
    }

    fclose(file);
    cols = strlen(grid[0]);

    Solution sol = solve(grid, word, rows, cols);

    if (sol.startRow == -1)
    {
        printf("Word not found.\n");
    }
    else
    {
        printf("(%d,%d),(%d,%d)\n", sol.startCol, sol.startRow, sol.endCol, sol.endRow);
        printGridWithWord(grid, rows, cols, sol, word);
    }

    return 0;
}


int main(int argc, char *argv[]) 
{
    if(argc > 4)
    {
        errx(EXIT_FAILURE, "too much arguments");
    }

    if (argc > 1 && strcmp(argv[1], "--mlp") == 0)
    {
        return run_mlp();
    }

    if (argc > 1 && strcmp(argv[1], "--solver") == 0)
    {
        if(argc >= 4)
        {
            return run_solver(argv[2], argv[3]);
        }
        else
        {
            errx(EXIT_FAILURE, "solver needs a file to process and a word to find");
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        errx(EXIT_FAILURE, "%s", SDL_GetError());
    }

    SDL_Renderer *renderer = NULL;
    SDL_Window *window = NULL;
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = NULL;

    char* file = NULL;
    if (argc >1)
    {
        file = argv[1];

    }

    initialize(&window, &renderer, &texture, file, &surface);

    int running = 1;
    int steps = 0;
    int av_gray = 0;

    printf("Press enter key to begin process\n");
    fflush(stdout);

    while (running)
    {
    running = event_handler(renderer, &texture, &surface, &steps, &av_gray, file);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    }

    SDL_FreeSurface(surface);
    terminate(window, renderer, texture);
    return EXIT_SUCCESS;
}
