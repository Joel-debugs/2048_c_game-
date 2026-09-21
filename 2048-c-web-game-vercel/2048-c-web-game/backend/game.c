#include "game.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static int board[SIZE][SIZE];
static int score = 0;
static GameStatus status = PLAYING;
static int rng_seeded = 0;

/* Seed the C random number generator once. */
static void seedRandom(void)
{
    if (!rng_seeded) {
        srand((unsigned int)time(NULL));
        rng_seeded = 1;
    }
}

/* Return a random empty cell through row/column pointers. */
static int findRandomEmptyCell(int *row, int *col)
{
    int empty[SIZE * SIZE][2];
    int count = 0;

    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            if (board[r][c] == 0) {
                empty[count][0] = r;
                empty[count][1] = c;
                count++;
            }
        }
    }

    if (count == 0) {
        return 0;
    }

    int index = rand() % count;
    *row = empty[index][0];
    *col = empty[index][1];
    return 1;
}

/* Add one tile: 2 has 90% probability, 4 has 10%. */
static void addRandomTile(void)
{
    int row, col;

    if (!findRandomEmptyCell(&row, &col)) {
        return;
    }

    board[row][col] = (rand() % 10 == 0) ? 4 : 2;
}

/*
 * Compress one row to the left.
 * Example: 0 2 0 2 -> 2 2 0 0
 */
static int compressRow(int row[SIZE])
{
    int temp[SIZE] = {0, 0, 0, 0};
    int index = 0;
    int changed = 0;

    for (int i = 0; i < SIZE; i++) {
        if (row[i] != 0) {
            temp[index++] = row[i];
        }
    }

    for (int i = 0; i < SIZE; i++) {
        if (row[i] != temp[i]) {
            changed = 1;
        }
        row[i] = temp[i];
    }

    return changed;
}

/*
 * Merge a compressed row from left to right.
 * A tile can merge only once in a move.
 */
static int mergeRow(int row[SIZE])
{
    int changed = 0;

    for (int i = 0; i < SIZE - 1; i++) {
        if (row[i] != 0 && row[i] == row[i + 1]) {
            row[i] *= 2;
            score += row[i];
            row[i + 1] = 0;
            changed = 1;
            i++; /* Skip the newly created tile so it cannot merge twice. */
        }
    }

    return changed;
}

static void moveRowLeft(int row[SIZE])
{
    compressRow(row);
    mergeRow(row);
    compressRow(row);
}

/* Compare two 4x4 boards. */
static int boardsEqual(const int a[SIZE][SIZE], const int b[SIZE][SIZE])
{
    return memcmp(a, b, sizeof(int) * SIZE * SIZE) == 0;
}

/*
 * Core left movement. We use a before-board so the public move functions
 * can determine whether anything actually changed.
 */
int moveLeft(void)
{
    int before[SIZE][SIZE];

    memcpy(before, board, sizeof(board));

    for (int r = 0; r < SIZE; r++) {
        moveRowLeft(board[r]);
    }

    if (boardsEqual(before, board)) {
        return 0;
    }

    addRandomTile();
    checkWin();

    if (status != WON && checkGameOver()) {
        status = GAME_OVER;
    }

    return 1;
}

int moveRight(void)
{
    int before[SIZE][SIZE];

    memcpy(before, board, sizeof(board));

    for (int r = 0; r < SIZE; r++) {
        int row[SIZE];

        for (int c = 0; c < SIZE; c++) {
            row[c] = board[r][SIZE - 1 - c];
        }

        moveRowLeft(row);

        for (int c = 0; c < SIZE; c++) {
            board[r][SIZE - 1 - c] = row[c];
        }
    }

    if (boardsEqual(before, board)) {
        return 0;
    }

    addRandomTile();
    checkWin();

    if (status != WON && checkGameOver()) {
        status = GAME_OVER;
    }

    return 1;
}

int moveUp(void)
{
    int before[SIZE][SIZE];

    memcpy(before, board, sizeof(board));

    for (int c = 0; c < SIZE; c++) {
        int line[SIZE];

        for (int r = 0; r < SIZE; r++) {
            line[r] = board[r][c];
        }

        moveRowLeft(line);

        for (int r = 0; r < SIZE; r++) {
            board[r][c] = line[r];
        }
    }

    if (boardsEqual(before, board)) {
        return 0;
    }

    addRandomTile();
    checkWin();

    if (status != WON && checkGameOver()) {
        status = GAME_OVER;
    }

    return 1;
}

int moveDown(void)
{
    int before[SIZE][SIZE];

    memcpy(before, board, sizeof(board));

    for (int c = 0; c < SIZE; c++) {
        int line[SIZE];

        for (int r = 0; r < SIZE; r++) {
            line[r] = board[SIZE - 1 - r][c];
        }

        moveRowLeft(line);

        for (int r = 0; r < SIZE; r++) {
            board[SIZE - 1 - r][c] = line[r];
        }
    }

    if (boardsEqual(before, board)) {
        return 0;
    }

    addRandomTile();
    checkWin();

    if (status != WON && checkGameOver()) {
        status = GAME_OVER;
    }

    return 1;
}

int checkWin(void)
{
    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            if (board[r][c] >= TARGET) {
                status = WON;
                return 1;
            }
        }
    }

    return 0;
}

int canMove(void)
{
    /* Empty cell means a move is potentially possible. */
    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            if (board[r][c] == 0) {
                return 1;
            }
        }
    }

    /* Check horizontal equal neighbors. */
    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE - 1; c++) {
            if (board[r][c] == board[r][c + 1]) {
                return 1;
            }
        }
    }

    /* Check vertical equal neighbors. */
    for (int r = 0; r < SIZE - 1; r++) {
        for (int c = 0; c < SIZE; c++) {
            if (board[r][c] == board[r + 1][c]) {
                return 1;
            }
        }
    }

    return 0;
}

int checkGameOver(void)
{
    if (canMove()) {
        return 0;
    }

    status = GAME_OVER;
    return 1;
}

int getScore(void)
{
    return score;
}

GameStatus getStatus(void)
{
    return status;
}

void getBoard(int out[SIZE][SIZE])
{
    memcpy(out, board, sizeof(board));
}

void initializeGame(void)
{
    seedRandom();

    memset(board, 0, sizeof(board));
    score = 0;
    status = PLAYING;

    addRandomTile();
    addRandomTile();
}

void resetGame(void)
{
    initializeGame();
}

/*
 * Deterministic helper for unit tests.
 * It is not used by the HTTP server.
 */
void setBoardForTest(const int input[SIZE][SIZE], int initialScore)
{
    memcpy(board, input, sizeof(board));
    score = initialScore;
    status = PLAYING;

    checkWin();

    if (status != WON && checkGameOver()) {
        status = GAME_OVER;
    }
}
