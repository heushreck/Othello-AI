#include "board.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


/* Base bitboard type*/
typedef unsigned __int128 bitboard_t;

/* Internal board_t structiure(hiden from the outside) */
struct board_t
{
  size_t size;
  disc_t player;
  bitboard_t black;
  bitboard_t white;
  bitboard_t moves;
  bitboard_t next_move;
  bitboard_t stable_black;
  bitboard_t stable_white;
};

static bitboard_t set_bitboard(const size_t size, const size_t row,
  const size_t column){
  bitboard_t board = 1;
  board = board << ((size * row) + column);
  return board;
}

static bitboard_t shift_north(const size_t size, const bitboard_t bitboard){
  return bitboard >> size;
}

static bitboard_t shift_south(const size_t size, const bitboard_t bitboard){
  bitboard_t board = bitboard << size;
  bitboard_t masc = ~0;
  masc = masc >> (128 - (size * size));
  board = board & masc;
  return board;
}

static bitboard_t shift_east(const size_t size, const bitboard_t bitboard){
  bitboard_t board = bitboard << 1;
  bitboard_t masc = set_bitboard(size, 0, 0);
  for(size_t i = 0; i < (size * size); i++){
    masc = masc << 1;
    if((i+1) % size != 0){
      masc += 1;
    }
  }
  board = board & masc;
  return board;
}

static bitboard_t shift_west(const size_t size, const bitboard_t bitboard){
  bitboard_t board = bitboard >> 1;
  bitboard_t masc = 0;
  for(size_t i = 0; i < (size * size); i++){
    masc = masc << 1;
    if(i % size != 0){
      masc += 1;
    }
  }
  board = board & masc;
  return board;
}

static bitboard_t shift_nw(const size_t size, const bitboard_t bitboard){
  bitboard_t board = shift_north(size, bitboard);
  board = shift_west(size, board);
  return board;
}

static bitboard_t shift_sw(const size_t size, const bitboard_t bitboard){
  bitboard_t board = shift_south(size, bitboard);
  board = shift_west(size, board);
  return board;
}

static bitboard_t shift_se(const size_t size, const bitboard_t bitboard){
  bitboard_t board = shift_south(size, bitboard);
  board = shift_east(size, board);
  return board;
}

static bitboard_t shift_ne(const size_t size, const bitboard_t bitboard){
  bitboard_t board = shift_north(size, bitboard);
  board = shift_east(size, board);
  return board;
}

static bitboard_t (*shift_direction[8])
(const size_t size, const bitboard_t bitboard);

bitboard_t bitboard_1;
bitboard_t bitboard_2;
bitboard_t bitboard_3;
bitboard_t bitboard_4;
bitboard_t bitboard_5;
bitboard_t bitboard_6;
bitboard_t bitboard_7;
bitboard_t bitboard_8;
bitboard_t stable_check;

/* from https://www.playingwithpointers.com/blog/swar.html */
static size_t pop_cnt_64(bitboard_t i){
  i = i - ((i >> 1) & 0x5555555555555555);
  i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
  i = (((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F) *
          0x0101010101010101);
  bitboard_t masc = 0;
  masc = ~masc;
  masc = masc >> 64;
  i = i & masc;
  return i >> 56;
}

static size_t bitboard_popcount(const bitboard_t bitboard){
  if(bitboard == 0){
    return 0;
  }
  bitboard_t count = bitboard;
  bitboard_t masc = 0;
  masc = ~masc;
  masc = masc >> 64;
  count = count & masc;
  size_t a = pop_cnt_64(count);
  count = bitboard;
  count = count >> 64;
  size_t b = pop_cnt_64(count);
  return a + b;
}

static bitboard_t compute_moves(const size_t size, const bitboard_t player,
  const bitboard_t opponent){
  bitboard_t moves = 0;
  for(int i = 0; i < 8; i++){
    bitboard_t candidates;
    candidates = opponent & ((*shift_direction[i]) (size, player));
    while(bitboard_popcount(candidates) > 0){
      moves = moves | ((~(player + opponent)) &
      ((*shift_direction[i]) (size, candidates)));
      candidates = opponent & ((*shift_direction[i]) (size, candidates));
    }
  }
  bitboard_t masc = 1;
  masc = masc << (size * size);
  masc = masc - 1;
  moves = moves & masc;
  return moves;
}

static void update_moves(board_t *board){
  bitboard_t player;
  bitboard_t opponent;
  if(board->player == BLACK_DISC){
    player = board->black;
    opponent = board->white;
  } else {
    player = board->white;
    opponent = board->black;
  }
  board->moves = compute_moves(board->size, player, opponent);
  board->next_move = board->moves;
}

size_t board_count_player_moves(board_t *board){
  return bitboard_popcount(board->moves);
}

bool board_is_move_valid(const board_t *board, const move_t move){
  bitboard_t moves = board->moves;
  return ((moves >> ((board->size * move.row) + move.column)) & 0x1);
}

static disc_t other_player(board_t *board){
  disc_t opponent;
  if(board->player == BLACK_DISC){
    opponent = WHITE_DISC;
  } else {
    opponent = BLACK_DISC;
  }
  return opponent;
}

bool board_stable_is_possible(board_t *board){
  return (bitboard_popcount((board->white | board->black) & stable_check) > 0);
}

int board_evaluat_discs(board_t *board, disc_t player){
  bitboard_t player_bitboard;
  bitboard_t opponent_bitboard;
  if(player == BLACK_DISC){
    player_bitboard = board->black;
    opponent_bitboard = board->white;
  } else {
    player_bitboard = board->white;
    opponent_bitboard = board->black;
  }
  int result = 0;
  result += (bitboard_popcount(player_bitboard & bitboard_1) * 20);
  result -= (bitboard_popcount(opponent_bitboard & bitboard_1) * 20);

  result += (bitboard_popcount(player_bitboard & bitboard_2) * -3);
  result -= (bitboard_popcount(opponent_bitboard & bitboard_2) * -3);

  result += (bitboard_popcount(player_bitboard & bitboard_3) * 11);
  result -= (bitboard_popcount(opponent_bitboard & bitboard_3) * 11);

  result += (bitboard_popcount(player_bitboard & bitboard_4) * 8);
  result -= (bitboard_popcount(opponent_bitboard & bitboard_4) * 8);

  result += (bitboard_popcount(player_bitboard & bitboard_5) * -7);
  result -= (bitboard_popcount(opponent_bitboard & bitboard_5) * -7);

  result += (bitboard_popcount(player_bitboard & bitboard_6) * -4);
  result -= (bitboard_popcount(opponent_bitboard & bitboard_6) * -4);

  result += (bitboard_popcount(player_bitboard & bitboard_7) * 1);
  result -= (bitboard_popcount(opponent_bitboard & bitboard_7) * 1);

  result += (bitboard_popcount(player_bitboard & bitboard_8) * 2);
  result -= (bitboard_popcount(opponent_bitboard & bitboard_8) * 2);

  return result;
}

int board_mobility(board_t *board, disc_t player){
  bitboard_t player_bitboard;
  bitboard_t opponent_bitboard;
  if(player == BLACK_DISC){
    player_bitboard = board->black;
    opponent_bitboard = board->white;
  } else {
    player_bitboard = board->white;
    opponent_bitboard = board->black;
  }
  size_t player_moves = bitboard_popcount(
    compute_moves(board->size, player_bitboard, opponent_bitboard));
  size_t opponent_moves = bitboard_popcount(
    compute_moves(board->size, opponent_bitboard, player_bitboard));
  return player_moves - opponent_moves;
}

static int board_frontiers_helper(board_t *board, disc_t player){
  bitboard_t player_bitboard;
  bitboard_t opponent_bitboard;
  if(player == BLACK_DISC){
    player_bitboard = board->black;
    opponent_bitboard = board->white;
  } else {
    player_bitboard = board->white;
    opponent_bitboard = board->black;
  }
  bitboard_t empty = ~(player_bitboard | opponent_bitboard);
  bitboard_t frontiers = 0;
  bitboard_t matched;

  matched = empty & ((*shift_direction[0]) (board->size, player_bitboard));
  frontiers |= (*shift_direction[1]) (board->size, matched);

  matched = empty & ((*shift_direction[1]) (board->size, player_bitboard));
  frontiers |= (*shift_direction[0]) (board->size, matched);

  matched = empty & ((*shift_direction[2]) (board->size, player_bitboard));
  frontiers |= (*shift_direction[3]) (board->size, matched);

  matched = empty & ((*shift_direction[3]) (board->size, player_bitboard));
  frontiers |= (*shift_direction[2]) (board->size, matched);

  matched = empty & ((*shift_direction[4]) (board->size, player_bitboard));
  frontiers |= (*shift_direction[7]) (board->size, matched);

  matched = empty & ((*shift_direction[7]) (board->size, player_bitboard));
  frontiers |= (*shift_direction[4]) (board->size, matched);

  matched = empty & ((*shift_direction[5]) (board->size, player_bitboard));
  frontiers |= (*shift_direction[6]) (board->size, matched);

  matched = empty & ((*shift_direction[6]) (board->size, player_bitboard));
  frontiers |= (*shift_direction[5]) (board->size, matched);

  return bitboard_popcount(frontiers);
}

int board_frontiers(board_t *board, disc_t player){
  int my_frontiers = board_frontiers_helper(board, player);
  disc_t opponent = EMPTY_DISC;
  if(player == BLACK_DISC){
    opponent = WHITE_DISC;
  } else {
    opponent = BLACK_DISC;
  }
  int opponent_frontiers = board_frontiers_helper(board, opponent);
  return -(my_frontiers - opponent_frontiers);
}

static void board_compute_stable_pieces_helper(board_t *board,
  size_t size, bool black){
  bitboard_t player;
  bitboard_t direction_1;
  bitboard_t direction_2;
  bitboard_t maybe_stable;
  bitboard_t stable;
  if(black){
    stable = board->stable_black;
    player = board->black;
  } else {
    stable = board->stable_white;
    player = board->white;
  }
  direction_1 = ((*shift_direction[0]) (size, player));
  direction_1 = direction_1 & ~(direction_1 & stable);
  direction_1 = ((*shift_direction[1]) (size, direction_1));
  direction_1 = direction_1 ^ player;

  direction_2 = ((*shift_direction[1]) (size, player));
  direction_2 = direction_2 & ~(direction_2 & stable);
  direction_2 = ((*shift_direction[0]) (size, direction_2));
  direction_2 = direction_2 ^ player;
  maybe_stable = direction_1 | direction_2;

  direction_1 = ((*shift_direction[2]) (size, player));
  direction_1 = direction_1 & ~(direction_1 & stable);
  direction_1 = ((*shift_direction[3]) (size, direction_1));
  direction_1 = direction_1 ^ player;

  direction_2 = ((*shift_direction[3]) (size, player));
  direction_2 = direction_2 & ~(direction_2 & stable);
  direction_2 = ((*shift_direction[2]) (size, direction_2));
  direction_2 = direction_2 ^ player;
  maybe_stable = maybe_stable & (direction_1 | direction_2);

  direction_1 = ((*shift_direction[4]) (size, player));
  direction_1 = direction_1 & ~(direction_1 & stable);
  direction_1 = ((*shift_direction[7]) (size, direction_1));
  direction_1 = direction_1 ^ player;

  direction_2 = ((*shift_direction[7]) (size, player));
  direction_2 = direction_2 & ~(direction_2 & stable);
  direction_2 = ((*shift_direction[4]) (size, direction_2));
  direction_2 = direction_2 ^ player;
  maybe_stable = maybe_stable & (direction_1 | direction_2);

  direction_1 = ((*shift_direction[5]) (size, player));
  direction_1 = direction_1 & ~(direction_1 & stable);
  direction_1 = ((*shift_direction[6]) (size, direction_1));
  direction_1 = direction_1 ^ player;

  direction_2 = ((*shift_direction[6]) (size, player));
  direction_2 = direction_2 & ~(direction_2 & stable);
  direction_2 = ((*shift_direction[5]) (size, direction_2));
  direction_2 = direction_2 ^ player;
  maybe_stable = maybe_stable & (direction_1 | direction_2);

  if(black){
    board->stable_black = board->stable_black | maybe_stable;
  } else {
    board->stable_white = board->stable_black | maybe_stable;
  }
}

void board_compute_stable_pieces(board_t *board){
  bool condition = true;
  bitboard_t check = board->stable_black;
  while(condition){
    board_compute_stable_pieces_helper(board, board->size, true);
    if(check == board->stable_black){
      condition = false;
    } else {
      check = board->stable_black;
    }
  }
  condition = true;
  check = board->stable_white;
  while(condition){
    board_compute_stable_pieces_helper(board, board->size, false);
    if(check == board->stable_white){
      condition = false;
    } else {
      check = board->stable_white;
    }
  }
}

int board_stable(board_t *board, disc_t player){
  board_compute_stable_pieces(board);
  int my_stable;
  int opponent_stable;
  if(player == BLACK_DISC){
    my_stable = bitboard_popcount(board->stable_black);
    opponent_stable = bitboard_popcount(board->stable_white);
  } else {
    my_stable = bitboard_popcount(board->stable_white);
    opponent_stable = bitboard_popcount(board->stable_black);
  }
  return my_stable - opponent_stable;
}

void board_check_end(board_t *board){
  if(board_count_player_moves(board) == 0){
    board_set_player(board, other_player(board));
    update_moves(board);
    if(board_count_player_moves(board) == 0){
      board_set_player(board, EMPTY_DISC);
    }
  }
}

size_t turns_left(board_t *board){
  size_t turn = bitboard_popcount(board->black) +
    bitboard_popcount(board->white);
  size_t board_size = board->size * board->size;
  return board_size - turn;
}

game_stage stage_of_game(board_t *board){
  size_t turn = bitboard_popcount(board->black) +
    bitboard_popcount(board->white);
  game_stage result;
  if(turn <= (board->size * board->size) / 3){
    result = EARLY_GAME;
  } else if(turn <= 2 * (board->size * board->size) / 3){
    result = MID_GAME;
  } else if(turn <= (board->size * board->size) - 5){
    result = END_GAME;
  } else {
    result = END_END_GAME;
  }
  return result;
}

bool board_play(board_t *board, const move_t move){
  if(!board_is_move_valid(board, move)){
    return false;
  }
  if(board->player == EMPTY_DISC){
    return false;
  }

  bitboard_t player;
  bitboard_t opponent;
  bitboard_t changes = 0;
  if(board->player == BLACK_DISC){
    player = board->black;
    opponent = board->white;
  } else {
    player = board->white;
    opponent = board->black;
  }
  board_set(board, board->player, move.row, move.column);
  /*here the code where the pieces are turned around*/
  for(int i = 0; i < 8; i++){
    bitboard_t maybe_changes = 0;
    bitboard_t maybe = ((*shift_direction[i])
    (board->size, set_bitboard(board->size, move.row, move.column)));
    while((maybe & opponent) != 0){
      maybe_changes |= maybe;
      maybe = ((*shift_direction[i]) (board->size, maybe));
    }
    if((maybe & player) != 0){
      changes |= maybe_changes;
    }
  }
  if(board->player == BLACK_DISC){
    board->black |= changes;
    board->white &= ~changes;
  } else {
    board->white |= changes;
    board->black &= ~changes;
  }
  board_set_player(board, other_player(board));
  update_moves(board);
  board_check_end(board);
  return true;
}

move_t board_next_move(board_t *board){
  if(board->next_move == 0){
    board->next_move = board->moves;
  }
  bitboard_t next_move = board->next_move;
  int last_bit = -1;

  for(size_t i = 0; i < (board->size * board->size); i++){
    if((next_move >> i) & 0x1){
      last_bit = i;
    }
  }
  if(last_bit == -1){
    move_t empty = { .row = 11, .column = 11 };
    return empty;
  } else {
    size_t column = last_bit % board->size;
    size_t row = last_bit / board->size;
    move_t result = { .row = row, .column = column };
    board->next_move =
      board->next_move & ~(set_bitboard(board->size, 0, 0) << last_bit);
    return result;
  }
}

static void first_time_things(size_t size){
  shift_direction[0] = shift_north;
  shift_direction[1] = shift_south;
  shift_direction[2] = shift_west;
  shift_direction[3] = shift_east;
  shift_direction[4] = shift_ne;
  shift_direction[5] = shift_nw;
  shift_direction[6] = shift_se;
  shift_direction[7] = shift_sw;

  /* stable_check serves to check if a stable piece is possible */
  stable_check = 1;
  stable_check = stable_check << (size - 1);
  stable_check += 1;
  stable_check = stable_check << (((size - 2) * size) + 1);
  stable_check += 1;
  stable_check = stable_check << (size - 1);
  stable_check += 1;

  /* the next couple of bitboards determin how good discs positions are.
   * Only works for board size 8
   * bitboard_1 has position 00, 07, 70, 77
    * and those positions have value 20 */
  bitboard_1 = 1;
  bitboard_1 = bitboard_1 << 7;
  bitboard_1 += 1;
  bitboard_1 = bitboard_1 << 49;
  bitboard_1 += 1;
  bitboard_1 = bitboard_1 << 7;
  bitboard_1 += 1;
  /* bitboard_2 has position 01, 06, 10, 17, 33, 34, 43, 44, 60, 67, 71, 76
   * and those positions have value -3 */
  bitboard_2 = 1;
  bitboard_2 = bitboard_2 << 5;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 2;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 7;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 12;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 1;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 7;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 1;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 12;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 7;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 2;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 5;
  bitboard_2 += 1;
  bitboard_2 = bitboard_2 << 1;
  /* bitboard_3 has position 02, 05, 20, 27, 50, 57, 72, 75
   * and those positions have value -3 */
  bitboard_3 = 1;
  bitboard_3 = bitboard_3 << 3;
  bitboard_3 += 1;
  bitboard_3 = bitboard_3 << 11;
  bitboard_3 += 1;
  bitboard_3 = bitboard_3 << 7;
  bitboard_3 += 1;
  bitboard_3 = bitboard_3 << 17;
  bitboard_3 += 1;
  bitboard_3 = bitboard_3 << 7;
  bitboard_3 += 1;
  bitboard_3 = bitboard_3 << 11;
  bitboard_3 += 1;
  bitboard_3 = bitboard_3 << 3;
  bitboard_3 += 1;
  bitboard_3 = bitboard_3 << 2;
  /* bitboard_4 has position 03, 04, 30, 37, 40, 47, 73, 74
   * and those positions have value 8 */
  bitboard_4 = 1;
  bitboard_4 = bitboard_4 << 1;
  bitboard_4 += 1;
  bitboard_4 = bitboard_4 << 20;
  bitboard_4 += 1;
  bitboard_4 = bitboard_4 << 7;
  bitboard_4 += 1;
  bitboard_4 = bitboard_4 << 1;
  bitboard_4 += 1;
  bitboard_4 = bitboard_4 << 7;
  bitboard_4 += 1;
  bitboard_4 = bitboard_4 << 20;
  bitboard_4 += 1;
  bitboard_4 = bitboard_4 << 1;
  bitboard_4 += 1;
  bitboard_4 = bitboard_4 << 3;
  /* bitboard_5 has position 11, 16, 61, 66
   * and those positions have value -7 */
  bitboard_5 = 1;
  bitboard_5 = bitboard_5 << 5;
  bitboard_5 += 1;
  bitboard_5 = bitboard_5 << 35;
  bitboard_5 += 1;
  bitboard_5 = bitboard_5 << 5;
  bitboard_5 += 1;
  bitboard_5 = bitboard_5 << 9;
  /* bitboard_6 has position 12, 15, 21, 26, 51, 56, 62, 65
   * and those positions have value -4 */
  bitboard_6 = 1;
  bitboard_6 = bitboard_6 << 3;
  bitboard_6 += 1;
  bitboard_6 = bitboard_6 << 4;
  bitboard_6 += 1;
  bitboard_6 = bitboard_6 << 5;
  bitboard_6 += 1;
  bitboard_6 = bitboard_6 << 19;
  bitboard_6 += 1;
  bitboard_6 = bitboard_6 << 5;
  bitboard_6 += 1;
  bitboard_6 = bitboard_6 << 4;
  bitboard_6 += 1;
  bitboard_6 = bitboard_6 << 3;
  bitboard_6 += 1;
  bitboard_6 = bitboard_6 << 10;
  /* bitboard_7 has position 13, 14, 31, 36, 41, 46, 63, 64
   * and those positions have value 1 */
  bitboard_7 = 1;
  bitboard_7 = bitboard_7 << 1;
  bitboard_7 += 1;
  bitboard_7 = bitboard_7 << 13;
  bitboard_7 += 1;
  bitboard_7 = bitboard_7 << 5;
  bitboard_7 += 1;
  bitboard_7 = bitboard_7 << 3;
  bitboard_7 += 1;
  bitboard_7 = bitboard_7 << 5;
  bitboard_7 += 1;
  bitboard_7 = bitboard_7 << 13;
  bitboard_7 += 1;
  bitboard_7 = bitboard_7 << 1;
  bitboard_7 += 1;
  bitboard_7 = bitboard_7 << 11;
  /* bitboard_8 has position 22, 23, 24, 25, 32,
   * 35, 42, 45, 52, 53, 54, 55
   * and those positions have value 2 */
  bitboard_8 = 15;
  bitboard_8 = bitboard_8 << 5;
  bitboard_8 += 1;
  bitboard_8 = bitboard_8 << 3;
  bitboard_8 += 1;
  bitboard_8 = bitboard_8 << 5;
  bitboard_8 += 1;
  bitboard_8 = bitboard_8 << 3;
  bitboard_8 += 1;
  bitboard_8 = bitboard_8 << 8;
  bitboard_8 += 15;
  bitboard_8 = bitboard_8 << 18;
}

board_t *board_alloc(const size_t size, const disc_t player,
  const bool first_time){
  if(first_time){
    /* first_time_things is called only once because of the variable
     * first_time, which is only true in board_init and the file_parser.
     * Not in board_copy. */
    first_time_things(size);
  }
  if(size >= MIN_BOARD_SIZE && size <= MAX_BOARD_SIZE && size % 2 == 0){
    bitboard_t black = 0;
    bitboard_t white = 0;
    bitboard_t moves = 0;
    bitboard_t next_move = 0;
    bitboard_t stable_black = 0;
    bitboard_t stable_white = 0;

    struct board_t * result = malloc(sizeof(struct board_t));
    if(!result){
      free(result);
      return NULL;
    }
    result->size = size;
    result->player = player;
    result->black = black;
    result->white = white;
    result->moves = moves;
    result->next_move = next_move;
    result->stable_black = stable_black;
    result->stable_white = stable_white;
    return result;
  } else {
    return NULL;
  }
}

void board_free(board_t *board){
  free(board);
}

board_t *board_init(const size_t size){
  disc_t player;
  if(size == 2){
    player = EMPTY_DISC;
  } else {
    player = BLACK_DISC;
  }
  struct board_t *reversi = board_alloc(size, player, true);
  if(reversi == NULL){
    return NULL;
  }
  board_set(reversi, WHITE_DISC,(size / 2) - 1,(size / 2) - 1);
  board_set(reversi, BLACK_DISC,(size / 2) - 1,(size / 2));
  board_set(reversi, BLACK_DISC,(size / 2),(size / 2) - 1);
  board_set(reversi, WHITE_DISC,(size / 2),(size / 2));
  reversi->moves = compute_moves(size, reversi->black, reversi->white);
  return reversi;
}

board_t *board_copy(const board_t *board){
  if(board == NULL){
    return NULL;
  }
  struct board_t * copy = board_alloc(board->size, board->player, false);
  copy->black = board->black;
  copy->white = board->white;
  copy->moves = board->moves;
  copy->next_move = board->next_move;
  copy->stable_black = board->stable_black;
  copy->stable_white = board->stable_white;
  return copy;
}

size_t board_size(const board_t *board){
  return board->size;
}

disc_t board_player(const board_t *board){
  return board->player;
}

void board_set_player(board_t *board, disc_t player){
  board->player = player;
}

disc_t board_get(const board_t *board, const size_t row, const size_t column){
  if(board == NULL){
    return EMPTY_DISC;
  }
  int shift = ((board->size * row) + column);
  bitboard_t bitboard = board->black >> shift;
  if(bitboard & 0x1){
    return BLACK_DISC;
  }
  bitboard = board->white >> shift;
  if(bitboard & 0x1){
    return WHITE_DISC;
  }
  bitboard = board->moves >> shift;
  if(bitboard & 0x1){
    return HINT_DISC;
  }
  return EMPTY_DISC;
}

void board_set(board_t *board, const disc_t disc, const size_t row,
  const size_t column){
  if(board != NULL && row < board->size && column < board->size){
    bitboard_t masc = set_bitboard(board->size, row, column);
    bitboard_t inverse_masc = ~masc;
    switch (disc){
      case BLACK_DISC:
        board->black = board->black | masc;
        board->white = board->white & inverse_masc;
        board->moves = board->moves & inverse_masc;
        break;
      case WHITE_DISC:
        board->white = board->white | masc;
        board->black = board->black & inverse_masc;
        board->moves = board->moves & inverse_masc;
        break;
      case EMPTY_DISC:
        board->black = board->black & inverse_masc;
        board->white = board->white & inverse_masc;
        break;
      case HINT_DISC:
        board->moves = board->moves | masc;
        break;
    }
    update_moves(board);
  }
}

score_t board_score(const board_t *board){
  unsigned short white = 0;
  unsigned short black = 0;
  white = bitboard_popcount(board->white);
  black = bitboard_popcount(board->black);
  score_t score = { .black = black, .white = white };
  return score;
}

int board_print(const board_t *board, FILE *fd){
  if(fd == NULL){
    printf("No file descriptor found\n");
    return -1;
  }
  if(board == NULL){
    printf("No board found\n");
    return -1;
  }
  int result = 0;
  result += fprintf(fd, "\n  ");
  for(size_t i = 'A'; i < ('A' + board->size); i++){
    result += fprintf(fd, " %c", (char) i);
  }
  result += fprintf(fd, "\n");
  for(size_t row = 0; row < board->size; ++row){
    if(row < 9){
      result += fprintf(fd, "%ld ", row + 1);
    } else {
      result += fprintf(fd, "%ld", row + 1);
    }
    for(size_t column = 0; column < board->size; ++column){
      disc_t disc = board_get(board, row, column);
      result += fprintf(fd, " %c", disc);
    }
    result += fprintf(fd, "\n");
  }
  score_t score = board_score(board);
  result += fprintf(fd, "\nScore: BLACK_DISC = %d, WHITE_DISC = %d\n",
  score.black, score.white);
  if(board->player != EMPTY_DISC){
    result += fprintf(fd, "\n'%c' player's turn.\n", board->player);
  }
  return result;
}
