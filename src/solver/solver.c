#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "solver.h"

#define RED   "\x1b[31m"
#define RESET "\x1b[0m"

int check(char grid[100][100], char *word, int i, int j, int dir0, int dir1, int r, int c, Solution *solu){
    int start_i = i;
    int start_j = j;
    int len = strlen(word);

    for (int o = 0; o<len; o++)
    {
        if( i<0 || i>=r || 0>j ||j >=c)
            return 0;
        if (grid[i][j]!= word[o])
        {
            return 0;
        }
        i+=dir0;
        j+=dir1;
    }
    solu->startRow=start_i;
    solu->startCol=start_j;
    solu->endRow=i-dir0;
    solu->endCol=j-dir1;
    return 1;
}

Solution solve(char grid[100][100], char *word, int rows, int cols){
    Solution solu = {-1, -1, -1, -1};
    int dir[8][2] = {{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};
    for (int i=0; i<rows; i++)
    {
        for (int j=0; j<cols; j++)
        {
            if (grid[i][j]==word[0])
            {
                for (int k=0; k<8; k++)
                {
                    if (check(grid, word, i, j, dir[k][0], dir[k][1], rows, cols, &solu) == 1)
                    {
                        return solu;
                    }
                }
            }
        }
    }
    return solu;
}

void printGridWithWord(char grid[100][100], int rows, int cols, Solution sol, const char *word) {
    int dirRow = (sol.endRow > sol.startRow) ? 1 : (sol.endRow < sol.startRow ? -1 : 0);
    int dirCol = (sol.endCol > sol.startCol) ? 1 : (sol.endCol < sol.startCol ? -1 : 0);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int isPartOfWord = 0;
            int len = strlen(word);

            for (int k = 0; k < len; k++) {
                int row = sol.startRow + k * dirRow;
                int col = sol.startCol + k * dirCol;
                if (i == row && j == col) {
                    isPartOfWord = 1;
                    break;
                }
            }

            if (isPartOfWord == 1)
                printf(RED "%c" RESET, grid[i][j]);
            else
                printf("%c", grid[i][j]);
        }
        printf("\n");
    }
}