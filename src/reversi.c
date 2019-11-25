#define _POSIX_C_SOURCE 200809L

#include "reversi.h"

#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <board.h>
#include <player.h>

static bool verbose = false;
static size_t black_tactic = 0;
static size_t white_tactic = 0;

static bool isSignificant(char c){
  return (c == EMPTY_DISC || c == BLACK_DISC || c == WHITE_DISC || c == '#');
}
static bool isUnsignificant(char c){
  return (c == ' ' || c == '\n' || c == '\t');
}

static board_t *file_parser(const char *filename){
  disc_t player = EMPTY_DISC;
  ssize_t read;
  int whatline = 0;
  size_t rows = 0;
  size_t size = 1;
  char board_string[MAX_BOARD_SIZE * MAX_BOARD_SIZE] = "";
  FILE *inputfile = fopen(filename, "r");

  if(!inputfile){
    fprintf(stderr, "reversi: error: Could not open the file %s\n", filename);
    exit(EXIT_FAILURE);
  }
  /* Getting first line */
  char *line = NULL;
  size_t line_size = 0;

  /* Reading */
  while((read = getline(&line, &line_size, inputfile)) != -1){
    if(line[0] == '\0'){
      continue;
    }
    whatline++;
    size_t row_size = 0;
    for(int i = 0; i < read; i++){
      if(isSignificant(line[i])){
        if(line[i] == '#'){
          break;
        }
        if(player == EMPTY_DISC){
          if(line[i] == BLACK_DISC || line[i] == WHITE_DISC){
            player = line[i];
          } else {
            fprintf(stderr, "reversi: error: player incorrect or missing\n");
            exit(EXIT_FAILURE);
          }
        } else {
          row_size++;
          char tmp[2];
        	tmp[0] = line[i];
        	tmp[1] = '\0';
          strcat(board_string, tmp);
        }
      } else if(isUnsignificant(line[i])){
        continue;
      } else {
        fprintf(stderr, "reversi: error: wrong character '%c' at line %d.\n",
        line[i], whatline);
        exit(EXIT_FAILURE);
      }
    }
    if(row_size == 0){
      continue;
    }
    if(row_size >= MIN_BOARD_SIZE && row_size <= MAX_BOARD_SIZE &&
      row_size % 2 == 0){
      rows++;
      if(size == 1){
        size = row_size;
      } else if(size != row_size){
        fprintf(stderr,
          "reversi: error: row %d is malformated!(wrong number of columns)",
        whatline);
        exit(EXIT_FAILURE);
      }
    } else {
      fprintf(stderr,
        "reversi: error: row %d is malformated!(wrong number of columns)\n",
      whatline);
      exit(EXIT_FAILURE);
    }
  }
  if(rows != size){
    if(rows > size){
      fprintf(stderr, "reversi: error: board has %ld extra row(s)\n",
      (rows-size));
    } else {
      fprintf(stderr, "reversi: error: board has %ld missing row(s)\n",
      (size-rows));
    }
    exit(EXIT_FAILURE);

  }
  fclose(inputfile);
  if(line){
    free(line);
  }
  struct board_t *reversi = board_alloc(size, player, true);
  int pointer = 0;
  for(size_t row = 0; row < size; ++row){
    for(size_t column = 0; column < size; ++column){
      board_set(reversi, board_string[pointer], row, column);
      pointer++;
    }
  }
  board_check_end(reversi);
  board_compute_stable_pieces(reversi);
  return reversi;
}

static int game(move_t (*black)(board_t*),
  move_t (*white)(board_t*), board_t *board){
  int result = 3;
  bool resigned = false;
  char* black_player = "random";
  char* white_player = "random";
  switch (black_tactic){
    case 0:
      black_player = "human";
      break;
    case 2:
      black_player = "AI";
      break;
  }
  switch (white_tactic){
    case 0:
      white_player = "human";
      break;
    case 2:
      white_player = "AI";
      break;
  }
  printf("%s\n", "Welcome to the best reversi game you will ever play!");
  printf("Black player (X) is %s and white player (O) is %s\n",
   black_player, white_player);
  if(board_count_player_moves(board) == 0){
    board_set_player(board, EMPTY_DISC);
  }
  if(board_player(board) == WHITE_DISC){
    printf("%s player starts!\n", "White");
  } else if(board_player(board) == BLACK_DISC){
    printf("%s player starts!\n", "Black");
  }
  disc_t current_player = EMPTY_DISC;
  while(board_player(board) != EMPTY_DISC){
    current_player = board_player(board);
    move_t move;
    board_print(board, stdout);
    if(current_player == BLACK_DISC){
      move = (*black)(board);
    } else {
      move = (*white)(board);
    }
    if(!board_play(board, move)){
      if(move.row == board_size(board) && move.column == board_size(board)){
        resigned = true;
      }
    } else {
      if(verbose){
        printf("\nMove %c%ld was played by player %c\n",
          (char) move.column + 'a',move.row + 1, current_player);
      }
    }
  }
  if(board_count_player_moves(board) == 0){
    score_t score = board_score(board);
    if(score.black > score.white){
      printf("Player '%c' wins the game.\n", BLACK_DISC);
      result = 1;
    } else if(score.black < score.white){
      printf("Player '%c' wins the game.\n", WHITE_DISC);
      result = 2;
    } else {
      printf("%s\n", "Draw game, no winner.");
      result = 0;
    }
  } else {
    if(resigned){
      if(current_player == BLACK_DISC){
        printf("Player '%c' resigned. Player '%c' wins the game.\n",
        BLACK_DISC, WHITE_DISC);
        result = -1;
      } else {
        printf("Player '%c' resigned. Player '%c' wins the game.\n",
        WHITE_DISC, BLACK_DISC);
        result = -2;
      }
    } else {
      printf("%s\n", "Game was saved.");
    }
  }
  board_print(board, stdout);
  printf("\n%s\n", "Thanks for playing, see you soon!");
  return result;
}

int main(int argc, char * const argv[]){
  size_t board_size = 8;
  bool contest_mode = false;

  move_t (*tactics[3]) (board_t *board);
  tactics[0] = human_player;
  tactics[1] = random_player;
  tactics[2] = ai_player;

  int optc;
  char* opts = "s:b::w::cvVh";

  struct option long_opts[] = {
    { "size", required_argument, NULL, 's' },
    { "black-ai", optional_argument, NULL, 'b' },
    { "white-ai", optional_argument, NULL, 'w' },
    { "contest", no_argument, NULL, 'c' },
    { "verbose", no_argument, NULL, 'v' },
    { "version", no_argument, NULL, 'V' },
    { "help", no_argument, NULL, 'h' },
    { NULL,       0,                 NULL,  0  }
  };

  while((optc = getopt_long(argc, argv, opts, long_opts, NULL)) != -1){
    switch(optc){
      case 's':
        if(atoi(optarg) >= 1 && atoi(optarg) <= 5){
          board_size = atoi(optarg) * 2;
          printf("Your size is now: %ld.\n", board_size);
        } else {
          printf("Your size was not right, it has to be an int between 1 and 5\n");
          return EXIT_FAILURE;
        }
        break;

      case 'b':
        if(optarg == NULL) break;
        if(atoi(optarg) >= 0 && atoi(optarg) <= 2){
          black_tactic = atoi(optarg);
          printf("Black plays now with tactic '%s'.\n", optarg);
        } else {
          printf("This tactic does not exist, it has to be an int between 0 and 3\n");
          return EXIT_FAILURE;
        }
        break;

      case 'w':
        if(optarg == NULL) break;
        if(atoi(optarg) >= 0 && atoi(optarg) <= 2){
          white_tactic = atoi(optarg);
          printf("White plays now with tactic '%s'.\n", optarg);
        } else {
          printf("This tactic does not exist, it has to be an int between 0 and 3\n");
          return EXIT_FAILURE;
        }
        break;

      case 'c':
        contest_mode = true;
        break;

      case 'v':
        verbose = true;
        printf("You set the global variable verbose to true.\n");
        break;

      case 'V':
        printf("reversi %d.%d.%d\nThis software allows to play to reversi game.\n",
        VERSION, SUBVERSION, REVERSI);
        return EXIT_SUCCESS;

      case 'h':
        printf(
          "Usage: reversi [-s SIZE|-b [N] |-w [N]|-c|-v|-V|-h] [FILE]\n"
          "Play a reversi game with human or program players\n"
          "-s, --size SIZE\t\tboard size(min=1, max=5(default=4))\n"
          "-b, --black-ai [N]\t\tset tactic of black player(default: 0)\n"
          "-w, --white-ai [N]\t\tset tactic of white player(default: 0)\n"
          "-c, --contest\t\t\tenable 'contest' mode\n"
          "-v, --verbose\t\t\tverbose output\n"
          "-V, --version\t\t\tdisplay version and exit\n"
          "-h, --help\t\t\tdisplay this help text\n"
          "\n"
          "Tactic list: human(0) random(1) ai(2)\n"
        );
        return EXIT_SUCCESS;

      default:
        printf("Type 'reversi --help' for more information.\n");
        return EXIT_FAILURE;
    }
  }
  struct board_t *board;
  if(contest_mode){
    if(argv[optind] != NULL && access(argv[optind], F_OK) == 0){
      board = file_parser(argv[optind]);
      size_t depth = 5;
      size_t how_mayn_turns_left = turns_left(board);
      if(how_mayn_turns_left < 10){
        depth = how_mayn_turns_left;
      }
      move_t move = minmax_ab_player(board, depth);
      printf("%c%ld\n",(char) move.column + 'a',move.row + 1);
    } else {
      void err();
      err(EXIT_FAILURE, "File unreadable");
    }
  } else {
    if(argv[optind] != NULL && access(argv[optind], F_OK) == 0){
      board = file_parser(argv[optind]);
    } else {
      board = board_init(board_size);
    }
    game((*tactics[black_tactic]),
      (*tactics[white_tactic]), board);
  }
  board_free(board);
  return EXIT_SUCCESS;
}
