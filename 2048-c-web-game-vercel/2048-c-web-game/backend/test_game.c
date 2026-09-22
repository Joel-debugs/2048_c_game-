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

static void test_move_up(void)
{
    const int input[SIZE][SIZE] = {
        {2, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {2, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    assert(moveUp() == 1);
    assert(getScore() == 4);

    int actual[SIZE][SIZE];
    getBoard(actual);

    assert(actual[0][0] == 4);
}

static void test_move_down(void)
{
    const int input[SIZE][SIZE] = {
        {2, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {2, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    assert(moveDown() == 1);
    assert(getScore() == 4);

    int actual[SIZE][SIZE];
    getBoard(actual);

    assert(actual[SIZE - 1][0] == 4);
}

/* A row with two independent merges must score both and not double-merge. */
static void test_multiple_merges_in_one_move(void)
{
    const int input[SIZE][SIZE] = {
        {2, 2, 4, 4},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    assert(moveLeft() == 1);
    assert(getScore() == 12); /* 2+2=4, 4+4=8, 4+8=12 */

    int actual[SIZE][SIZE];
    getBoard(actual);

    assert(actual[0][0] == 4);
    assert(actual[0][1] == 8);
}

/* Vertical moves that cannot change the board must report no-op and leave
 * score/board untouched, mirroring test_invalid_move_does_not_change_score
 * but for the up/down axis. */
static void test_invalid_vertical_move_does_not_change_state(void)
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

    assert(moveUp() == 0);
    assert(getScore() == 0);

    int after[SIZE][SIZE];
    getBoard(after);

    assert(memcmp(before, after, sizeof(before)) == 0);
}

/* Winning must not stop the engine: subsequent valid moves keep playing,
 * keep accumulating score, and status must remain WON. */
static void test_can_continue_after_win(void)
{
    const int input[SIZE][SIZE] = {
        {1024, 1024, 0, 0},
        {2,    2,    0, 0},
        {0,    0,    0, 0},
        {0,    0,    0, 0}
    };

    setBoardForTest(input, 0);
    assert(moveLeft() == 1);
    assert(getStatus() == WON);
    assert(getScore() == 2048 + 4);

    /* Row 1 (2,2,0,0) is already left-compacted/merged, but row 0 becoming
     * (2048,0,0,0) plus a freshly spawned tile means a further left move
     * on this board is a no-op; use a fresh, clearly-movable WON board
     * instead to check that play continues past a win. */
    const int input2[SIZE][SIZE] = {
        {2048, 0, 2, 2},
        {0,    0, 0, 0},
        {0,    0, 0, 0},
        {0,    0, 0, 0}
    };

    setBoardForTest(input2, 2048);
    assert(getStatus() == WON);

    assert(moveLeft() == 1);
    assert(getStatus() == WON); /* still WON, not reset to PLAYING */
    assert(getScore() == 2048 + 4);

    int actual[SIZE][SIZE];
    getBoard(actual);
    assert(actual[0][0] == 2048);
    assert(actual[0][1] == 4);
}

/* getBoard() must hand back a copy: mutating the caller's array must not
 * corrupt the engine's internal state. */
static void test_get_board_returns_a_copy(void)
{
    const int input[SIZE][SIZE] = {
        {2, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);

    int snapshot[SIZE][SIZE];
    getBoard(snapshot);
    snapshot[0][0] = 9999; /* mutate the caller's copy */

    int again[SIZE][SIZE];
    getBoard(again);

    assert(again[0][0] == 2); /* engine's own board must be unaffected */
}

int main(void)
{
    test_double_merge_prevention();
    test_score();
    test_invalid_move_does_not_change_score();
    test_win();
    test_game_over();
    test_direction();
    test_move_up();
    test_move_down();
    test_multiple_merges_in_one_move();
    test_invalid_vertical_move_does_not_change_state();
    test_can_continue_after_win();
    test_get_board_returns_a_copy();

    printf("All C 2048 engine tests passed.\n");
    return 0;
}