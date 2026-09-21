/*
 * Deterministic tests for the C 2048 engine.
 *
 * Windows:
 *   gcc test_game.c game.c -o test_game.exe
 *   .\test_game.exe
 */

#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_double_merge_prevention(void)
{
    const int input[SIZE][SIZE] = {
        {2, 2, 2, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    assert(moveLeft() == 1);

    /*
     * A random tile is added after a valid move, so only assert the
     * deterministic first two cells and that the score is exactly 4.
     */
    int actual[SIZE][SIZE];
    getBoard(actual);

    assert(actual[0][0] == 4);
    assert(actual[0][1] == 2);
    assert(getScore() == 4);
}

static void test_score(void)
{
    const int input[SIZE][SIZE] = {
        {4, 4, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    assert(moveLeft() == 1);
    assert(getScore() == 8);
}

static void test_invalid_move_does_not_change_score(void)
{
    const int input[SIZE][SIZE] = {
        {2, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);

    int before[SIZE][SIZE];
    getBoard(before);

    assert(moveLeft() == 0);
    assert(getScore() == 0);

    int after[SIZE][SIZE];
    getBoard(after);

    assert(memcmp(before, after, sizeof(before)) == 0);
}

static void test_win(void)
{
    const int input[SIZE][SIZE] = {
        {1024, 1024, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    assert(getStatus() == PLAYING);

    moveLeft();

    assert(getStatus() == WON);
    assert(getScore() == 2048);
}

static void test_game_over(void)
{
    const int input[SIZE][SIZE] = {
        {2, 4, 2, 4},
        {4, 2, 4, 2},
        {2, 4, 2, 4},
        {4, 2, 4, 8}
    };

    setBoardForTest(input, 0);

    assert(canMove() == 0);
    assert(checkGameOver() == 1);
    assert(getStatus() == GAME_OVER);
}

static void test_direction(void)
{
    const int input[SIZE][SIZE] = {
        {2, 0, 0, 2},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    assert(moveRight() == 1);
    assert(getScore() == 4);

    int actual[SIZE][SIZE];
    getBoard(actual);

    assert(actual[0][3] == 4);
}

int main(void)
{
    test_double_merge_prevention();
    test_score();
    test_invalid_move_does_not_change_score();
    test_win();
    test_game_over();
    test_direction();

    printf("All C 2048 engine tests passed.\n");
    return 0;
}
