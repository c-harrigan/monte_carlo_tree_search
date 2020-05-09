CC=g++
CFLAGS=-I
mcts: puzzlefile.h mcts.cpp
	$(CC) -o mcts puzzlefile.h mcts.cpp
clean:
	rm mcts
