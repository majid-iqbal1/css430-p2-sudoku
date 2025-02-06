// Sudoku puzzle verifier and solver

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int type;
  int index;
  int startRow;
  int startCol;
  int psize;
  int n;
  int valid;
  int **grid;
} ThreadData;

void *validateRegion(void *param) {
  ThreadData *data = (ThreadData *)param;
  int psize = data->psize;
  int *found = calloc(psize + 1, sizeof(int));
  if (data->type == 0) {
    int row = data->index;
    for (int col = 1; col <= psize; col++) {
      int num = data->grid[row][col];
      if (num < 1 || num > psize) {
        data->valid = 0;
        free(found);
        pthread_exit(NULL);
      }
      if (found[num] == 1) {
        data->valid = 0;
        free(found);
        pthread_exit(NULL);
      }
      found[num] = 1;
    }
    data->valid = 1;
  } else if (data->type == 1) {
    int col = data->index;
    for (int row = 1; row <= psize; row++) {
      int num = data->grid[row][col];
      if (num < 1 || num > psize) {
        data->valid = 0;
        free(found);
        pthread_exit(NULL);
      }
      if (found[num] == 1) {
        data->valid = 0;
        free(found);
        pthread_exit(NULL);
      }
      found[num] = 1;
    }
    data->valid = 1;
  } else if (data->type == 2) {
    // Validate a subgrid. The top-left cell is at (startRow, startCol)
    for (int row = data->startRow; row < data->startRow + data->n; row++) {
      for (int col = data->startCol; col < data->startCol + data->n; col++) {
        int num = data->grid[row][col];
        if (num < 1 || num > psize) {
          data->valid = 0;
          free(found);
          pthread_exit(NULL);
        }
        if (found[num] == 1) {
          data->valid = 0;
          free(found);
          pthread_exit(NULL);
        }
        found[num] = 1;
      }
    }
    data->valid = 1;
  }
  free(found);
  pthread_exit(NULL);
}

// takes puzzle size and grid[][] representing sudoku puzzle
// and tow booleans to be assigned: complete and valid.
// row-0 and column-0 is ignored for convenience, so a 9x9 puzzle
// has grid[1][1] as the top-left element and grid[9]9] as bottom right
// A puzzle is complete if it can be completed with no 0s in it
// If complete, a puzzle is valid if all rows/columns/boxes have numbers from 1
// to psize For incomplete puzzles, we cannot say anything about validity
void checkPuzzle(int psize, int **grid, bool *complete, bool *valid) {
  int n = (int)(sqrt(psize) + 0.5);

  bool progress;
  do {
    progress = false;
    for (int row = 1; row <= psize; row++) {
      int missingCount = 0;
      int missingCol = -1;
      bool present[psize + 1];
      for (int i = 1; i <= psize; i++)
        present[i] = false;
      for (int col = 1; col <= psize; col++) {
        int num = grid[row][col];
        if (num == 0) {
          missingCount++;
          missingCol = col;
        } else {
          present[num] = true;
        }
      }
      if (missingCount == 1) {
        for (int num = 1; num <= psize; num++) {
          if (!present[num]) {
            grid[row][missingCol] = num;
            progress = true;
            break;
          }
        }
      }
    }

    for (int col = 1; col <= psize; col++) {
      int missingCount = 0;
      int missingRow = -1;
      bool present[psize + 1];
      for (int i = 1; i <= psize; i++)
        present[i] = false;
      for (int row = 1; row <= psize; row++) {
        int num = grid[row][col];
        if (num == 0) {
          missingCount++;
          missingRow = row;
        } else {
          present[num] = true;
        }
      }
      if (missingCount == 1) {
        for (int num = 1; num <= psize; num++) {
          if (!present[num]) {
            grid[missingRow][col] = num;
            progress = true;
            break;
          }
        }
      }
    }

    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        int startRow = i * n + 1;
        int startCol = j * n + 1;
        int missingCount = 0;
        int missingRow = -1, missingCol = -1;
        bool present[psize + 1];
        for (int k = 1; k <= psize; k++)
          present[k] = false;
        for (int row = startRow; row < startRow + n; row++) {
          for (int col = startCol; col < startCol + n; col++) {
            int num = grid[row][col];
            if (num == 0) {
              missingCount++;
              missingRow = row;
              missingCol = col;
            } else {
              present[num] = true;
            }
          }
        }
        if (missingCount == 1) {
          for (int num = 1; num <= psize; num++) {
            if (!present[num]) {
              grid[missingRow][missingCol] = num;
              progress = true;
              break;
            }
          }
        }
      }
    }
  } while (progress);

  bool isComplete = true;
  for (int row = 1; row <= psize && isComplete; row++) {
    for (int col = 1; col <= psize; col++) {
      if (grid[row][col] == 0) {
        isComplete = false;
        break;
      }
    }
  }
  *complete = isComplete;

  if (!isComplete) {
    *valid = false;
    return;
  }

  int totalThreads = 3 * psize;
  ThreadData *tdArray = malloc(totalThreads * sizeof(ThreadData));
  pthread_t *threads = malloc(totalThreads * sizeof(pthread_t));
  int threadIndex = 0;

  for (int r = 1; r <= psize; r++) {
    tdArray[threadIndex].type = 0;
    tdArray[threadIndex].index = r;
    tdArray[threadIndex].psize = psize;
    tdArray[threadIndex].grid = grid;
    tdArray[threadIndex].n = n;
    threadIndex++;
  }

  for (int c = 1; c <= psize; c++) {
    tdArray[threadIndex].type = 1;
    tdArray[threadIndex].index = c;
    tdArray[threadIndex].psize = psize;
    tdArray[threadIndex].grid = grid;
    tdArray[threadIndex].n = n;
    threadIndex++;
  }

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      tdArray[threadIndex].type = 2;
      tdArray[threadIndex].psize = psize;
      tdArray[threadIndex].grid = grid;
      tdArray[threadIndex].n = n;
      tdArray[threadIndex].startRow = i * n + 1;
      tdArray[threadIndex].startCol = j * n + 1;
      threadIndex++;
    }
  }

  for (int i = 0; i < totalThreads; i++) {
    int rc =
        pthread_create(&threads[i], NULL, validateRegion, (void *)&tdArray[i]);
    if (rc) {
      fprintf(stderr, "Error: pthread_create failed\n");
      exit(EXIT_FAILURE);
    }
  }

  bool overallValid = true;
  for (int i = 0; i < totalThreads; i++) {
    pthread_join(threads[i], NULL);
    if (tdArray[i].valid == 0)
      overallValid = false;
  }
  *valid = overallValid;

  free(tdArray);
  free(threads);
}

// takes filename and pointer to grid[][]
// returns size of Sudoku puzzle and fills grid
int readSudokuPuzzle(char *filename, int ***grid) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Could not open file %s\n", filename);
    exit(EXIT_FAILURE);
  }
  int psize;
  fscanf(fp, "%d", &psize);
  int **agrid = (int **)malloc((psize + 1) * sizeof(int *));
  for (int row = 1; row <= psize; row++) {
    agrid[row] = (int *)malloc((psize + 1) * sizeof(int));
    for (int col = 1; col <= psize; col++) {
      fscanf(fp, "%d", &agrid[row][col]);
    }
  }
  fclose(fp);
  *grid = agrid;
  return psize;
}

// takes puzzle size and grid[][]
// prints the puzzle
void printSudokuPuzzle(int psize, int **grid) {
  printf("%d\n", psize);
  for (int row = 1; row <= psize; row++) {
    for (int col = 1; col <= psize; col++) {
      printf("%d ", grid[row][col]);
    }
    printf("\n");
  }
  printf("\n");
}

//// takes puzzle size and grid[][]
// frees the memory allocated
void deleteSudokuPuzzle(int psize, int **grid) {
  for (int row = 1; row <= psize; row++) {
    free(grid[row]);
  }
  free(grid);
}

// expects file name of the puzzle as argument in command line
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("usage: ./sudoku puzzle.txt\n");
    return EXIT_FAILURE;
  }
  // grid is a 2D array
  int **grid = NULL;
  // find grid size and fill grid
  int sudokuSize = readSudokuPuzzle(argv[1], &grid);
  bool valid = false;
  bool complete = false;
  checkPuzzle(sudokuSize, grid, &complete, &valid);
  printf("Complete puzzle? %s\n", complete ? "true" : "false");
  if (complete) {
    printf("Valid puzzle? %s\n", valid ? "true" : "false");
  }
  printSudokuPuzzle(sudokuSize, grid);
  deleteSudokuPuzzle(sudokuSize, grid);
  return EXIT_SUCCESS;
}