#ifndef GAME_H
#define GAME_H

#define SIZE 4
#define TARGET 2048

typedef enum {
    PLAYING,
    WON,
    GAME_OVER
} GameStatus;

/* Starts a new game: clears the board, resets score/status, adds two tiles. */
void initializeGame(void);
void resetGame(void);

/* Direction functions return 1 if the board changed, 0 otherwise. */
int moveLeft(void);
int moveRight(void);
int moveUp(void);
int moveDown(void);

/* State/query functions. */
int checkWin(void);
int canMove(void);
int checkGameOver(void);
int getScore(void);
GameStatus getStatus(void);

/* Copies the current C engine board into the supplied 4x4 array. */
void getBoard(int out[SIZE][SIZE]);

/*
 * Test/helper function. It is useful for deterministic unit tests and does
 * not participate in normal frontend gameplay.
 */
void setBoardForTest(const int input[SIZE][SIZE], int score);

#endif
