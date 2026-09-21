/*
 * Deterministic tests for the C 2048 engine.
 *
 * Unlike bare assert(), CHECK() reports every failure (file/line/message)
 * and keeps running the rest of the suite, then prints a pass/fail summary
 * and exits non-zero if anything failed (useful for CI).
 *
 * Build & run:
 *   gcc -Wall -Wextra -std=c11 test_game.c game.c -o test_game
 *   ./test_game
 *
 * Windows:
 *   gcc test_game.c game.c -o test_game.exe
 *   .\test_game.exe
 */

#include "game.h"

#include <stdio.h>
#include <string.h>

static int checks_run = 0;
static int checks_failed = 0;
static int tests_run = 0;
static const char *current_test = "";

#define CHECK(cond)                                                     \
    do {                                                                \
        checks_run++;                                                  \
        if (!(cond)) {                                                  \
            checks_failed++;                                            \
            printf(                                                    \
                "  FAIL [%s] %s:%d: %s\n",                              \
                current_test, __FILE__, __LINE__, #cond                 \
            );                                                          \
        }                                                               \
    } while (0)

#define RUN_TEST(fn)                                                    \
    do {                                                                \
        current_test = #fn;                                             \
        tests_run++;                                                    \
        fn();                                                           \
    } while (0)

/* Count non-zero cells on the current board. */
static int countTiles(void)
{
    int board[SIZE][SIZE];
    int count = 0;

    getBoard(board);

    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            if (board[r][c] != 0) {
                count++;
            }
        }
    }

    return count;
}

static void test_double_merge_prevention(void)
{
    /* 2 2 2 0 -> merges the leftmost pair only: 4 2 0 0 (+random tile) */
    const int input[SIZE][SIZE] = {
        {2, 2, 2, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    CHECK(moveLeft() == 1);

    int actual[SIZE][SIZE];
    getBoard(actual);

    /*
     * A random tile is added after a valid move, so only assert the
     * deterministic cells and the score.
     */
    CHECK(actual[0][0] == 4);
    CHECK(actual[0][1] == 2);
    CHECK(getScore() == 4);
}

static void test_multi_merge_in_one_row(void)
{
    /* 2 2 4 4 -> 4 8 0 0, score += 4 + 8 */
    const int input[SIZE][SIZE] = {
        {2, 2, 4, 4},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    CHECK(moveLeft() == 1);

    int actual[SIZE][SIZE];
    getBoard(actual);

    CHECK(actual[0][0] == 4);
    CHECK(actual[0][1] == 8);
    CHECK(getScore() == 12);
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
    CHECK(moveLeft() == 1);
    CHECK(getScore() == 8);
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

    CHECK(moveLeft() == 0);
    CHECK(getScore() == 0);

    int after[SIZE][SIZE];
    getBoard(after);

    CHECK(memcmp(before, after, sizeof(before)) == 0);
}

static void test_no_op_when_already_compacted(void)
{
    /* Already left-aligned with no equal neighbors: nothing should move. */
    const int input[SIZE][SIZE] = {
        {2, 4, 8, 16},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);

    int before[SIZE][SIZE];
    getBoard(before);

    CHECK(moveLeft() == 0);

    int after[SIZE][SIZE];
    getBoard(after);

    CHECK(memcmp(before, after, sizeof(before)) == 0);
}

static void test_move_right(void)
{
    const int input[SIZE][SIZE] = {
        {2, 0, 0, 2},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    CHECK(moveRight() == 1);
    CHECK(getScore() == 4);

    int actual[SIZE][SIZE];
    getBoard(actual);

    CHECK(actual[0][3] == 4);
}

static void test_move_up(void)
{
    /* Column 0: 2 2 0 0 (top to bottom) -> 4 0 0 0, score += 4 */
    const int input[SIZE][SIZE] = {
        {2, 0, 0, 0},
        {2, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    CHECK(moveUp() == 1);
    CHECK(getScore() == 4);

    int actual[SIZE][SIZE];
    getBoard(actual);

    CHECK(actual[0][0] == 4);
    CHECK(actual[1][0] == 0);
}

static void test_move_down(void)
{
    /* Column 3: 0 0 8 8 (top to bottom) -> 0 0 0 16, score += 16 */
    const int input[SIZE][SIZE] = {
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 8},
        {0, 0, 0, 8}
    };

    setBoardForTest(input, 0);
    CHECK(moveDown() == 1);
    CHECK(getScore() == 16);

    int actual[SIZE][SIZE];
    getBoard(actual);

    CHECK(actual[3][3] == 16);
    CHECK(actual[2][3] == 0);
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
    CHECK(getStatus() == PLAYING);

    moveLeft();

    CHECK(getStatus() == WON);
    CHECK(getScore() == 2048);
}

static void test_win_detected_immediately_by_setBoardForTest(void)
{
    /* checkWin() should fire even without a move, straight off setBoardForTest. */
    const int input[SIZE][SIZE] = {
        {2048, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 0);
    CHECK(getStatus() == WON);
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

    CHECK(canMove() == 0);
    CHECK(checkGameOver() == 1);
    CHECK(getStatus() == GAME_OVER);
}

static void test_canMove_true_with_empty_cell(void)
{
    const int input[SIZE][SIZE] = {
        {2, 4, 2, 4},
        {4, 2, 4, 2},
        {2, 4, 2, 4},
        {4, 2, 4, 0} /* one empty cell */
    };

    setBoardForTest(input, 0);
    CHECK(canMove() == 1);
    CHECK(getStatus() == PLAYING);
}

static void test_canMove_true_with_horizontal_pair(void)
{
    const int input[SIZE][SIZE] = {
        {2, 2, 4, 8},
        {4, 8, 16, 32},
        {8, 16, 32, 64},
        {16, 32, 64, 128}
    };

    setBoardForTest(input, 0);
    CHECK(canMove() == 1);
    CHECK(getStatus() == PLAYING);
}

static void test_canMove_true_with_vertical_pair(void)
{
    const int input[SIZE][SIZE] = {
        {2, 4, 8, 16},
        {2, 8, 16, 32},
        {4, 16, 32, 64},
        {8, 32, 64, 128}
    };

    setBoardForTest(input, 0);
    CHECK(canMove() == 1);
    CHECK(getStatus() == PLAYING);
}

static void test_no_move_possible_on_full_board(void)
{
    const int input[SIZE][SIZE] = {
        {2, 4, 2, 4},
        {4, 2, 4, 2},
        {2, 4, 2, 4},
        {4, 2, 4, 8}
    };

    setBoardForTest(input, 0);

    /* No direction should report a change once the board is dead. */
    CHECK(moveLeft() == 0);
    CHECK(moveRight() == 0);
    CHECK(moveUp() == 0);
    CHECK(moveDown() == 0);
    CHECK(getStatus() == GAME_OVER);
}

static void test_initializeGame_starts_clean(void)
{
    initializeGame();

    CHECK(getScore() == 0);
    CHECK(getStatus() == PLAYING);
    CHECK(countTiles() == 2);
}

static void test_resetGame_matches_initializeGame(void)
{
    /* Get into a non-trivial state first. */
    const int input[SIZE][SIZE] = {
        {1024, 1024, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };

    setBoardForTest(input, 5000);
    CHECK(getStatus() == PLAYING);

    resetGame();

    CHECK(getScore() == 0);
    CHECK(getStatus() == PLAYING);
    CHECK(countTiles() == 2);
}

static void test_getBoard_returns_a_copy(void)
{
    initializeGame();

    int snapshot[SIZE][SIZE];
    getBoard(snapshot);

    /* Mutate the caller's copy; internal engine state must be unaffected. */
    snapshot[0][0] = 9999;

    int again[SIZE][SIZE];
    getBoard(again);

    CHECK(again[0][0] != 9999);
}

int main(void)
{
    RUN_TEST(test_double_merge_prevention);
    RUN_TEST(test_multi_merge_in_one_row);
    RUN_TEST(test_score);
    RUN_TEST(test_invalid_move_does_not_change_score);
    RUN_TEST(test_no_op_when_already_compacted);
    RUN_TEST(test_move_right);
    RUN_TEST(test_move_up);
    RUN_TEST(test_move_down);
    RUN_TEST(test_win);
    RUN_TEST(test_win_detected_immediately_by_setBoardForTest);
    RUN_TEST(test_game_over);
    RUN_TEST(test_canMove_true_with_empty_cell);
    RUN_TEST(test_canMove_true_with_horizontal_pair);
    RUN_TEST(test_canMove_true_with_vertical_pair);
    RUN_TEST(test_no_move_possible_on_full_board);
    RUN_TEST(test_initializeGame_starts_clean);
    RUN_TEST(test_resetGame_matches_initializeGame);
    RUN_TEST(test_getBoard_returns_a_copy);

    printf(
        "\n%d/%d checks passed (%d test functions run).\n",
        checks_run - checks_failed,
        checks_run,
        tests_run
    );

    if (checks_failed > 0) {
        printf("%d check(s) FAILED.\n", checks_failed);
        return 1;
    }

    printf("All C 2048 engine tests passed.\n");
    return 0;
}