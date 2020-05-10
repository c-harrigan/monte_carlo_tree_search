#include <iostream>
#include <ctime>
#include <cstring>

using namespace std;

int expanded = 0;
double memory = 0;

const char map[4] = {'U', 'D', 'L', 'R'};

/*Fifteen_puzzle class:
	the fifteen_puzzle class is just a representation of a puzzle using an array
	The class has various methods that need to be used by the Monte-Carlo Tree Search
*/
class fifteen_puzzle{
	private:
		int puzzle[16];
		//checks if two puzzles are equal
		int equals(fifteen_puzzle n){
			for(int i = 0; i < 16; i++){
				if(n.puzzle[i] != puzzle[i])	return 0;
			}
			return 1;
		}
		/*this function tests whether a move is successful by deciding the probability
		  of a successful move based on a function and the move type, as given by
		  the current location of the empty tile and the move direction. then a random 
		  roll decides if the move was successful given the odds provided for each move*/
		int swap_success(int zero_tile, char move){
			int move_id = move;
			//the calculation below for odds returns an integer ranging
			//from 1 to 85, by messing around with it, im happy with the average
			//odds of a successful move being 36. these integers can be thought of
			//as percentages
			int odds = ((15 + ((int)move % (zero_tile + 5))
				   *((int)move % (zero_tile + 4))) % 100);
			//this rand() line relies on the main program already seeding rand()
			int roll = rand()%100 + 1;
			if(roll < odds)	return 0;	//returns 0 if the random roll was lower than odds of success
			return 1;	//returns 1 if random roll met or exceeded odds for a given move
		}
		//private constructor used by avg_heuristic for testing purposes
		void scramble(){
			int temp;
			int r;
			for(int i = 0; i < 16; i++)	puzzle[i] = i;
			for(int i = 0; i < 16; i++){
				r = rand()%16;
				int temp = puzzle[i];
				puzzle[i] = puzzle[r];
				puzzle[r] = temp;
			}
		}

	public:
		//standard constructor
		fifteen_puzzle(){
			for(int i = 0; i < 16; i++)	puzzle[i] = 0;
		}

		//constructor that returns a new puzzle given an old puzzle and a move to make on it
		//this is used to create children in the tree, to successfull swaps are needed
                fifteen_puzzle(fifteen_puzzle p, char a){
                        //puzzle = new int[16];
                        for(int i = 0; i < 16; i++)
                                puzzle[i] = p.at(i);
			if(!valid_swap(a)){
				exit(0);
			}	
                        while(!(swap(a)));
                }

		//same as constructor above, but using an integer index for letters
		//0 is U, 1 is D, 2 is L, 3 is R
		fifteen_puzzle(fifteen_puzzle p, int index){
			//puzzle = new int[16];
			for(int i = 0; i < 16; i++)
				puzzle[i] = p.at(i);
			while(!(swap(map[index])));//exit(0);
		}
		//copy constructor
		fifteen_puzzle(const fifteen_puzzle& p){
			//puzzle = new int[16];
			for(int i = 0; i < 16; i++){
				puzzle[i] = p.at(i);
			}
		}
		//equals operator overload
		fifteen_puzzle operator=(const fifteen_puzzle& p){
			fifteen_puzzle q(p);
			return q;
		}

		//overload that allows for integer input on method below
		int valid_swap(int move) const{
			return valid_swap(map[move]);
		}
		//method which checks if swap criteria is valid (0 for invalid, 1 for valid)
		//not to be confused with swap_success, which decides if an
		//already valid swap is randomly successful
		int valid_swap(char a) const{
			sanity(102);	
			//finding empty tile
			int i = 0;
			for(i = 0; i < 16; i++)
				if(puzzle[i] == 0)	break;
			switch(a){
				//to move up, cannot be in top row
				case 'u':
				case 'U':
					if(i < 4)	return 0;
					break;
				//to move down, cannot be in bottom row
				case 'd':
				case 'D':
					if(i > 11)	return 0;
					break;
				//to move left, cannot be in leftmost column
				case 'l':
				case 'L':
					if(i % 4 == 0)	return 0;
					break;
				//to move right, cannot be in rightmost column
				case 'r':
				case 'R':
					if(i % 4 == 3)	return 0;
					break;
				//if letter is entered that is not a valid move
				//letter, then the move cannot be valid
				default:
					return 0;
			}
			sanity(133);
			//if switch is exited, move is valid given board configuration
			return 1;
		}	
		//simple getter that returns value at a given index
		int at(int index) const{
			if(index < 0 || index > 15)	return -1;
			return puzzle[index];
		}
		//prints out puzzle board
		void print() const{
			if(puzzle == NULL){
				cout<<"NULL puzzle sent to print"<<endl;
				return;
			}
                        for(int i = 0; i < 16; i++){
                                cout<<puzzle[i]<<"\t";
                                if(i%4 == 3) cout<<"\n";
                        }
			cout<<endl;
                }
		//method testing for goal
		bool goal_test() const{
			for(int i = 0; i < 16; i++)
				if(puzzle[i] != i + 1)	return false;
			return true;
		}
//NOTE: Heuristic function is subject to change, currently returns 0 for non-goal states,
//it may return some small value for near-goal states in the future
		//method that returns a heuristic value associated with the puzzle configuration
		//UCB1 is only convergent
                double heuristic() const{
			bool goal = true;
                        for(int i = 0; i < 16; i++)
                                if(puzzle[i] != i + 1)
                                        goal = false;
			
			if(goal)	return 1;
                        //if not a goal, calculates and returns heuristic
			int heuristic = 0;
			/*since mcts will assign value and higher value is better,
		 	  the heuristic used will be an inverse manhatten distance
			  scoring, where more points will be given to tiles closer 
			  to their proper placement.
			
			  The maximum manhatten distance in a 4x4 puzzle is 6, if a tile's
			  designated space is one of the corners, and its current placement
			  is the opposite corner. So the heuristic will be 6 - manhatten distance
			
			  however, since the values will compound in the tree search, smaller 
			  values are needed to avoid overflow
			*/
			int manhatten = 0;
			double value = 0;
			int column_difference = 0;
			int row_difference = 0;
			/*this loop computes each manhatten distance in constant time
			  then adds  their inverses to value;
			  the computation uses division and the mod operator to find
			  the differences between row and column between a tile's current 
			  index (i) and its correct location (v-1), as defined by:
			  |(v-1) % 4 - i % 4| + |(v-1) / 4 + i / 4|
			*/
			for(int i = 0; i < 16; i++){
				//skipping empty tile
				if(puzzle[i] == 0)	continue;
				//line that computes absolute value (v-1 % 4 + i % 4)
				column_difference = (i % 4 > (puzzle[i] - 1) % 4) 
						    ? i % 4 - (puzzle[i] - 1) % 4
						    : (puzzle[i] - 1) % 4 - i % 4;
				//line that computes absolute value (v-1 / 4 + i / 4)
                                row_difference = (i / 4 > (puzzle[i] - 1) / 4) 
                                                    ? i / 4 - (puzzle[i] - 1) / 4
                                                    : (puzzle[i] - 1) / 4 - i / 4;
				manhatten = row_difference + column_difference;
				value += (6.0 - manhatten);
				
			}
			//to avoid overly-large values that may overflow, divides down
			//return value/160
			return value/1600;
                }

		
		//swaps two pieces on the board using U, D, L, R for Up, Down, Left, and Right
		//returns 0 for unsuccessful swaps,
		//which is decided nondeterministically by the swap_success function
		int swap(char action){
			sanity(218);
			//first checking if valid move
			if(!valid_swap(action))	return 0;
			
			//finding 0/empty tile
                        int i = 0;
                        int temp;
                        for (i = 0; i < 16; i++)
                                if(puzzle[i] == 0) break;
                        temp = puzzle[i];

			//checking if the move is a failure or not
			if(!swap_success(i, action))	return 0;
			//if not a failure, perform the swap below

                        //switch statement which swaps empty tile based on action
                        switch(action){
                                case 'U':
                                        puzzle[i] = puzzle[i-4];
                                        puzzle[i-4] = temp;
                                        break;
                                case 'D':
                                        puzzle[i] = puzzle[i+4];
                                        puzzle[i+4] = temp;
                                        break;
                                case 'L':
                                        puzzle[i] = puzzle[i-1];
                                        puzzle[i-1] = temp;
                                        break;
                                case 'R':
                                        puzzle[i] = puzzle[i+1];
                                        puzzle[i+1] = temp;
                                default:
                                        break;
                        }
			return 1;
                }
		
		//copies from a sent puzzle
		void copy(const fifteen_puzzle& p){
			sanity(263);
			//clear();
			//puzzle = new int[16];
			for(int i = 0; i< 16; i++){
				puzzle[i] = p.at(i);
			}
		}

		//contructor for a root fifteen_puzzle ggiven a first puzzle to copy
                fifteen_puzzle(int* input){
                        //puzzle = new int[16];
			if(input != NULL){
				for(int i = 0; i < 16; i++)
					puzzle[i] = input[i];
			}
                }
		//debugging method
		void sanity(int line) const{
			for(int i = 0; i < 16; i++){
				if(puzzle[i] > 15 || puzzle[i] < 0){
					cout<<"PUZZLE FAILED SANITY CHECK ON LINE "
					    <<line<<". PRINTING AND EXITING."<<endl;
					print();
					exit(0);
				}
			}
		}
		//method only for testing, may be removed later
		void avg_heuristic(){
			//this method loops through testing random board configurations to find
			//the average value being returned by heuristic()
			int iterations = 10000;
			fifteen_puzzle p();
			double total;
			for(int i = 0; i < iterations; i++){
				p.scramble();
				total += p.heuristic();
			}
			cout<<"AVERAGE HEURISTIC OVER "<<iterations<<" ITERATIONS IS "<<total/iterations;
		}
		
};


//function which gets input and creates the start fifteen_puzzle's puzzle
int* get_input(int * b){
	string input;
	int num;
	cout<<"Enter sequence:\n";
	for(int i = 0; i < 16; i++){
		cin >> num;
		b[i] = num;
	}
	return b;
}

