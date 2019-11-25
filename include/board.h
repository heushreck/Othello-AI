#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#ifndef BOARD_H
#define BOARD_H

/* Min/Max width board */
#define MIN_BOARD_SIZE 2
#define MAX_BOARD_SIZE 10

/* Max int possible */
#define MAX_INT 214748366

/* Board discs */
typedef enum {
  BLACK_DISC = 'X',
  WHITE_DISC = 'O',
  EMPTY_DISC = '_',
  HINT_DISC = '*'
} disc_t;

/* Stage of Game*/
typedef enum {
  EARLY_GAME,
  MID_GAME,
  END_GAME,
  END_END_GAME
} game_stage;

/* A move in the reversi game */
typedef struct
{
  size_t row;
  size_t column;
} move_t;

/* Store the score of a game */
typedef struct
{
  unsigned short black;
  unsigned short white;
} score_t;

/* Reversi board (forward declaration to hide the implementation) */
typedef struct board_t board_t;

/* board_alloc allocates a new board */
board_t *board_alloc(const size_t size, const disc_t player,
  const bool first_time);

/* frees the previusly allocated board */
void board_free(board_t *board);

/* initiates the board with the starting discs. Player 'X' starts */
board_t *board_init(const size_t size);

/* returns a deep copy of the board */
board_t *board_copy(const board_t *board);

/* returns the board size */
size_t board_size(const board_t *board);

/* returns the current board player */
disc_t board_player(const board_t *board);

/* sets the current board player */
void board_set_player(board_t *board, disc_t player);

/* returns the disc at a specific position on the board
 * returns also HINT discs, and returns EMPTY_DISC if anything went wrong */
disc_t board_get(const board_t *board, const size_t row, const size_t column);

/* sets a disc_t at a specific position on the board.
 * Also removes all other discs which were in the same position */
void board_set(board_t *board, const disc_t disc, const size_t row,
  const size_t column);

/* returns the current board score */
score_t board_score(const board_t *board);

/* return how many turns until the board is completetd with discs */
size_t turns_left(board_t *board);

/* returns the current game stage */
game_stage stage_of_game(board_t *board);

/* return an evaluation of the boards discs depending on the player */
int board_evaluat_discs(board_t *board, disc_t player);

/* evaluates if a stable disc is possible by this point in the game */
bool board_stable_is_possible(board_t *board);

/* evaluates the mobility of the board for a given player */
int board_mobility(board_t *board, disc_t player);

/* evaluates the frontiers of the board for a given player */
int board_frontiers(board_t *board, disc_t player);

/* evaluates the stable discs of a board for a current player */
int board_stable(board_t *board, disc_t player);

/* returns the amount of possible moves a player has */
size_t board_count_player_moves(board_t *board);

/* checks if a move is valid, returns true if so, false otherwise */
bool board_is_move_valid(const board_t *board, const move_t move);

/* checks if there are any more moves for the current player and if not,
 * changes the player and looks again.
  * Gets called by board_play and file_parser */
void board_check_end(board_t *board);

/* sets the variable stable_black and stable_white with their pieces.
 * is called by board_stable and file_parser */
void board_compute_stable_pieces(board_t *board);

/* performs a move and its consequences.
 * Returns true if succes, false otherwise */
bool board_play(board_t *board, const move_t move);

/* returns a possible move for the player module to analyse.
 * Delets move afterwards out of the next_move bitboard.
 * Starts at the back of the bitboard and works its way to the front. */
move_t board_next_move(board_t *board);

/* prints the current board on the given file descriptor */
int board_print(const board_t *board, FILE *fd);

#endif /* BOARD_H */
