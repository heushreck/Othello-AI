# Variables
EXE=reversi

# Usual compilation flags
CFLAGS=-std=c11 -Wall -Wextra -g
CPPFLAGS=-I../include -DDEBUG
LDFLAGS=

# Special rules and targets
.PHONY: all clean help

# Rules and targets
all: $(EXE)

$(EXE): reversi.o board.o player.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

board.o: board.c ../include/board.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c board.c

player.o: player.c ../include/player.h ../include/board.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c player.c

reversi.o: reversi.c reversi.h ../include/board.h ../include/player.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

clean:
	@rm -f *~ *.o $(EXE)

help:
	@echo "Usage:"
	@echo " make all\t\tRuns the whole build of reversi"
	@echo " make reversi\t\tBuilds the executable file from reversi.c"
	@echo " make clean\t\tRemove all files generated by make"
	@echo " make help\t\tDisplay this help"
