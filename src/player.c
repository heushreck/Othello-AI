#define _POSIX_C_SOURCE 200809L

#include "player.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <board.h>

static bool random_is_initialized = false;

static void remove_spaces(char *s){
  int i,k = 0;
  for(i = 0; s[i]; i++){
    s[i]=s[i+k];
    if(s[i] == ' '|| s[i] == '\t' || s[i] == '\n'){
      k++;
      i--;
    }
  }
}

static void game_save(board_t *board){
  if(board == NULL){
    void err();
    err(EXIT_FAILURE, "No board found\n");
  }
  FILE *fd;
  printf("%s", "Give a filename to save the game (default: 'board.txt'):");
  char *line = NULL;
  size_t line_size = 0;

  if(!getline(&line, &line_size, stdin)){
    fprintf (stderr, "reversi: error: failed to read first line\n");
    exit (EXIT_FAILURE);
  }

  if(line[0] == '\n'){
    fd = fopen("board.txt", "w");
  } else {
    remove_spaces(line);
    fd = fopen(line, "we");
  }
  free(line);

  if(fd == NULL){
    void err();
    err(EXIT_FAILURE, "No file descriptor found\n");
  }
  fprintf(fd, "%c\n", board_player(board));

  for(size_t row = 0; row < board_size(board); ++row){
    for(size_t column = 0; column < board_size(board); ++column){
      disc_t disc = board_get(board, row, column);
      if(disc == HINT_DISC){
        disc = EMPTY_DISC;
      }
      fprintf(fd, "%c ", disc);
    }
    fprintf(fd, "\n");
  }
  fclose(fd);
}

move_t human_player(board_t *board){
  move_t result = (move_t) { board_size(board), board_size(board)};
  if(board == NULL){
    return result;
  }
  bool condition = true;
  while(condition){
    printf("\n%s",
      "Give your move (e.g. 'A5' or 'a5'), press 'q' or 'Q' to quit: ");
    char column_1 = ' ';
    char column_2 = ' ';
    size_t row = 11;
    char *line = NULL;
    size_t line_size = 0;
    if(getline(&line, &line_size, stdin)){
      remove_spaces(line);
      if(2 == sscanf(line, "%c%ld", &column_1, &row) &&
      (line[3] == '\0' || line[4] == '\0')){
        if(row < 1 || row > board_size(board)){
          printf("%s\n", "Column out of bounds. Wrong input, try again!");
        } else {
          size_t column_int = column_1;
          if(!(column_int >= 'a' && column_int < (board_size(board) + 'a')) &&
            !(column_int >= 'A' && column_int < (board_size(board) + 'A'))){
            printf("%s\n", "Row out of bounds. Wrong input, try again!");
          } else {
            if(column_int > 'A' + MAX_BOARD_SIZE){
              column_int = column_int - 'a';
            } else {
              column_int = column_int - 'A';
            }
            result = (move_t) { (row - 1), column_int };
            if(board_is_move_valid(board, result)){
              condition = false;
            } else {
              printf("%s\n", "This move is invalid. Wrong input, try again!");
            }
          }
        }
      } else if(1 == sscanf(line, "%c", &column_2) && line[2] == '\0'){
        if(column_2 == 'q' || column_2 == 'Q'){
          condition = false;
          printf("%s", "Quiting, do you want to save this game (y/N)? ");
          char answer = ' ';
          char *save = NULL;
          size_t save_size = 0;
          if(getline(&save, &save_size, stdin)){
            remove_spaces(save);
            if(1 == sscanf(save, "%c", &answer) && save[2] == '\0'){
              if(answer == 'y' || answer == 'Y'){
                result = (move_t) { MAX_BOARD_SIZE + 1, MAX_BOARD_SIZE + 1};
                game_save(board);
              }
            } else {
              result = (move_t) { board_size(board), board_size(board)};
            }
            free(save);
          }
          board_set_player(board, EMPTY_DISC);
        } else {
          printf("%s\n", "Wrong input, try again!");
        }
      } else {
        printf("%s\n", "Wrong input, try again!");
      }
      free(line);
    } else {
      printf("%s\n", "Wrong input, try again!");
    }
  }
  return result;
}

static void random_initialize(){
  if(!random_is_initialized){
    srand(time(0));
    random_is_initialized = true;
  }
}

static int score_heuristic(board_t *board, disc_t player){
  int result;
  if(board == NULL || player == EMPTY_DISC){
    /* returns an int to high to be returned normaly */
    return ((MAX_BOARD_SIZE * MAX_BOARD_SIZE) + 1);
  }
  score_t score = board_score(board);
  if(player == BLACK_DISC){
    result = score.black - score.white;
  } else {
    result = score.white - score.black;
  }
  return result;
}

static int final_heuristic(board_t *board, disc_t player){
  game_stage stage = stage_of_game(board);
  int score_weight = 1;
  int mobility_weight = 1;
  int stable_weight = 1;
  int disc_evaluation_weight = 1;
  int frontiers_weight = 1;
  switch (stage) {
    case EARLY_GAME:
      score_weight = 1;
      mobility_weight = 8;
      stable_weight = 20;
      disc_evaluation_weight = 7;
      frontiers_weight = 3;
      break;
    case MID_GAME:
      score_weight = 3;
      mobility_weight = 2;
      stable_weight = 10;
      disc_evaluation_weight = 4;
      frontiers_weight = 3;
      break;
    case END_GAME:
      score_weight = 7;
      mobility_weight = 10;
      stable_weight = 2;
      disc_evaluation_weight = 1;
      frontiers_weight = 2;
      break;
    case END_END_GAME:
      score_weight = 10;
      mobility_weight = 2;
      stable_weight = 0;
      disc_evaluation_weight = 0;
      frontiers_weight = 0;
  }

  int score = score_heuristic(board, player);
  int mobility = board_mobility(board, player);
  int stable = 0;
  if(board_stable_is_possible(board)){
    stable = board_stable(board, player);
  }
  int disc_evaluation = 0;
  if(board_size(board) == 8){
    disc_evaluation = board_evaluat_discs(board, player);
  }
  int frontiers = board_frontiers(board, player);

  return (score_weight * score) + (disc_evaluation * disc_evaluation_weight) +
    (mobility * mobility_weight) + (stable * stable_weight) +
    (frontiers * frontiers_weight);
}

move_t random_player(board_t *board){
  move_t result = (move_t) { board_size(board), board_size(board)};
  if(board == NULL){
    return result;
  }
  random_initialize();
  int r = rand() % board_count_player_moves(board);
  result = board_next_move(board);
  for(int i = 0; i < r; i++){
    result = board_next_move(board);
  }
  return result;
}

static int minimax_help(board_t *board, size_t depth, bool maximizingPlayer,
  disc_t player, bool ab, int a, int b, time_t timer){
  int value = final_heuristic(board, player);
  if(depth == 0 || (time(NULL) - timer) >= MAX_TIME){
    return value;
  }
  if(board_player(board) == EMPTY_DISC){
    int score = score_heuristic(board, player);
    if(score > 0){
      return 10000 - (10 * depth);
    } else  if(score < 0){
      return -(10000 + (10 * depth));
    } else {
      return -(5000 + (10 * depth));
    }
  }
  if(maximizingPlayer){
    bool maximizingPlayer = false;
    value = -MAX_INT;
    for(size_t i = 0; i < board_count_player_moves(board); i++){
      struct board_t *new_board = board_copy(board);
      move_t new_move = board_next_move(board);
      board_play(new_board, new_move);
      if(board_player(board) == board_player(new_board)){
        maximizingPlayer = true;
      }
      int new_value;
      if(ab){
        new_value = minimax_help(new_board, depth - 1, maximizingPlayer,
          player, false, a, b, timer);
        if(new_value > value){
          value = new_value;
        }
        if(value > a){
          a = value;
        }
        if(a > b){
          break;
        }
      } else {
        new_value = minimax_help(new_board, depth - 1, maximizingPlayer,
          player, false, 0, 0, timer);
        if(new_value > value){
          value = new_value;
        }
      }
      board_free(new_board);
    }
    return value;
  } else {
    bool maximizingPlayer = true;
    value = MAX_INT;
    for(size_t i = 0; i < board_count_player_moves(board); i++){
      struct board_t *new_board = board_copy(board);
      move_t new_move = board_next_move(board);
      board_play(new_board, new_move);
      if(board_player(board) == board_player(new_board)){
        maximizingPlayer = true;
      }
      int new_value;
      if(ab){
        new_value = minimax_help(new_board, depth - 1, maximizingPlayer,
          player, false, a, b, timer);
        if(new_value < value){
          value = new_value;
        }
        if(value < b){
          b = value;
        }
        if(a > b){
          break;
        }
      } else {
        new_value = minimax_help(new_board, depth - 1, maximizingPlayer,
          player, false, 0, 0, timer);
        if(new_value < value){
          value = new_value;
        }
      }
      board_free(new_board);
    }
    return value;
  }
}

static move_t minimax_player_help(board_t *board, size_t depth, bool ab){
  move_t return_move = (move_t) { MAX_BOARD_SIZE + 1, MAX_BOARD_SIZE + 1};
  if(depth == 0 || board == NULL){
    return return_move;
  }
  int value = -MAX_INT;
  bool maximizingPlayer = false;
  time_t timer = time(NULL);
  for(size_t i = 0; i < board_count_player_moves(board); i++){
    struct board_t *new_board = board_copy(board);
    move_t new_move = board_next_move(board);
    board_play(new_board, new_move);
    if(board_player(board) == board_player(new_board)){
      maximizingPlayer = true;
    }
    int new_value;
    if(ab){
      new_value = minimax_help(new_board, depth - 1, maximizingPlayer,
        board_player(board), true, value,
        ((MAX_BOARD_SIZE * MAX_BOARD_SIZE) + 1), timer);
    } else {
      new_value = minimax_help(new_board, depth - 1, maximizingPlayer,
        board_player(board), false, 0, 0, timer);
    }
    if(new_value > value){
      return_move = new_move;
      value = new_value;
    }
    board_free(new_board);
  }
  return return_move;
}

move_t minmax_player(board_t *board, size_t depth){
  return minimax_player_help(board, depth, false);
}

move_t minmax_ab_player(board_t *board, size_t depth){
  return minimax_player_help(board, depth, true);
}

move_t ai_player(board_t *board){
  size_t depth = 4;
  size_t how_mayn_turns_left = turns_left(board);
  if(how_mayn_turns_left < 10){
    depth = how_mayn_turns_left;
  }
  return minmax_ab_player(board, depth);
}
