#ifndef SOLVER_H
#define SOLVER_H

typedef struct {
    int startRow;
    int startCol;
    int endRow;
    int endCol;
} Solution;

int check(char grid[100][100], char *word, int i, int j, int dir0, int dir1, int r, int c, Solution *solu);
Solution solve(char grid[100][100], char *word, int rows, int cols);
void printGridWithWord(char grid[100][100], int rows, int cols, Solution sol, const char *word);

#endif