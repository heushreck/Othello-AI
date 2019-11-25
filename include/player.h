#ifndef PLAYER_H
#define PLAYER_H

#include <board.h>

/* MAX_TIME is the maximum time a AI has to make a move in sec */
#define MAX_TIME 29

/* A player function move_t (*player_func) (board_t *) returns a
 * chosen move depending on the given board. */

/* returns a move_t that a human enters over stdin
 * can also call save the game
 * return the row and column the human chose or
 * row=size and column=size if a player resigned
 * row=MAX_BOARD_SIZE+1 and column=MAX_BOARD_SIZE+1 if game was saved */
move_t human_player(board_t *board);

/* returns a random move from all moves possible moves */
move_t random_player(board_t *board);

/* calls the minmax_player methode with a depth of 3 */
move_t ai_player(board_t *board);

/* evaluates the best move
 * according to the minimax tree search algorithm up to a given depth */
move_t minmax_player(board_t *board, size_t depth);

/* evaluates the best move according to the
 * minimax tree search algorithm with ab prunning up to a given depth */
move_t minmax_ab_player(board_t *board, size_t depth);

#endif /* PLAYER_H */
