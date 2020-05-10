Monte Carlo Tree Search on a Nondeterministic 15-Puzzle

Files to run are:
	mcts.cpp
	puzzlefile.h
	Makefile
	input.txt
To run: 
	make
	./mcts < input.txt

Manipulating input:
	The first line may be changed to any scrambling of the numbers 0 through 15
	all separated by a space

	On the next line type y for display updates as the algorithm picks moves, n otherwise

	On the third line type n or no seeded rng, otherwise type a seed to use
