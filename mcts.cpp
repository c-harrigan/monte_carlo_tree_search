
/* 	mcts.cpp written by Conor Harrigan
	This program implements the monte-carlo tree search algorithm
	in order to solve a fifteen puzzle, working alongside puzzlefile.h

	The algorithm has four steps:
		1. Traversing
			The algorithm traverses over already-explored elements
			of the tree using UCB1 criteria, defined in the UCB1 method.
			After following these criteria to a leaf node, the second step starts
		2. Expansion
			Once a leaf node is reached, it has either been unvisited or not.
			If unvisited, step three starts. If visited, then all children for
			possible actions are expanded and added to the tree. In this 
			implementation, there are four actions that any node can take
			(although on occasion fewer, as the boundaries of the puzzle have
			fewer valid moves). After expanding children nodes and selecting
			one, step three begins.
		3. Random Walk
			Now at a leaf node, the algorithm begins to randomly select moves 
			and follow down a random path. In this implementation the criteria
			for stopping the random walk is a set limit of iterations.
		4. Backpropagate
			Now at some deep internal node, all the estimated values achieved at each 
			node are computed and set back up the chain to the starting leaf node
			and eventually the root. This is implemented in a two step process with
			the first step being the temporary  random walk nodes, done 
			recursively back to the leaf node, and the second step being the nodes 
			on the tree back to the root.
*/

#include "puzzlefile.h"
#include <iostream>
#include <ctime>
#include <cstring>
#include <cmath>
#include <cfloat>


#define RANDOM_WALK_ITERATIONS 200
#define MCTS_ITERATIONS 20
#define C_CONST 2.0

using namespace std;

/*      Random walk function
                        This function recursively creates temporary puzzles that are
                        not linked to any Node. It randomly picks a move to make and 
			proceeds down the walk. I opted to make this a function and
			not a method because the Node architecture is unnecessary as no
			tree needs to be stored or used.
		
                        There are two possible failures in proceeding:
                                1. move randomly selected was invalid
                                2. move randomly selected was valid, but the valid move
                                   nondeterministically failed
                        In the first case the algorithm wont be punished since a smarter
                        implementation could avoid this
                        However in the second case the whole iteration will fail, and miss out
                        on the value provided had the move been successful, so the program will 
                        learn from low-probalility moves that the expected value is lower
*/
double random_walk(int iterations, fifteen_puzzle state){
	//base case, number of iterations reached
	if(iterations < 0)	return 0.0;
	//randomly selecting a move until one is valid
	int move = rand()%4;
	while(!state.valid_swap(map[move]))	move = rand()%4;
	//now with a valid move, time to see if it was randomly successful
	if(!state.swap(map[move])){
		//within if, move was unsuccessful, so try again and miss out on 
		//an iteration of returns
		return random_walk(iterations - 1, state);
	}
	//outside if, swap was successful return current value plus value of further iterations
	double result = state.heuristic();
	result += random_walk(iterations - 1, state);
	return result;
}


/*	Node class:
	This class is used by the monte carlo tree search and is the
	implementation of the tree. Each node will typically have
	four children, though nodes representing the "sides" of the puzzle
	will have three, and the nodes representing the "corners" will have two.
*/
class Node{
	private:
		//array for child pointers, will always be an array of four
		//even though not every node will have four children
		Node *children;
		//tracking number of times a node is visited (updated during backpropagation)
		int visits;
		//tracking total value of a node (updated during backpropagation)
		double total_val;
		//parent pointer
		Node* parent;
		//puzzle held within each node representing the state
		fifteen_puzzle state;
		bool valid;
		bool leaf;
	public:
		//method to return a pointer to one of a Node's children
		Node getChild(int index){
			return children[index];
		}
		//method to retrieve number of visits
		int getVisits(){
			return visits;
		}
		//method to retrieve total value
		double getValue(){
			return total_val;
		}
		//method to retrieve state
		fifteen_puzzle getState(){
			return state;
		}
		//constructor that copies over a state and takes in a parent pointer
		Node(Node* par, fifteen_puzzle p){
			children = NULL;
			state.copy(p);
			total_val = state.heuristic();
			visits = 0;
			parent = par;
			valid = true;
			leaf = true;
		}
		//basic constructor, used to create invalid children
		Node(){
			children = NULL;
			visits = INT8_MAX;
			total_val = 0;
			parent = NULL;
			state = NULL;
			valid = false;
			leaf = false;
		}
		//constructor for root node, given a starting fifteen puzzle
		Node(fifteen_puzzle p){
			children = NULL;
			visits = 0;
			total_val = p.heuristic();
			parent = NULL;
			state.copy(p);
			valid = true;
			leaf = true;
		}
		Node operator=(const Node& n){
			visits = 0;
			total_val = n.state.heuristic();
			state.copy(n.state);
			parent = n.parent;
			leaf = true;
			children = NULL;
			valid = true;
		}
		//method to check if a Node is a goal node, using goal_test in puzzlefile
		bool is_goal(){
			if(state.goal_test())	return true;
			return false;
		}
		//method to expand children out, using a constructor
		void expand(){
			fifteen_puzzle p;
			children = new Node[4];
			//looping through and adding all children, specifying if theyre valid or not
			for(int i = 0; i < 4; i++){
				//if a move is invalid, sent to basic constructor
				if(!state.valid_swap(map[i])){
					children[i] = Node();
					continue;
				}
				children[i] = Node(this, fifteen_puzzle(state, map[i]));
			}
			leaf = false;
		}
		/*	UCB1 method returns the UCB1 value associated with a Node
			The UUCB1 formula is as follows:
			avg value + C*sqrt(lnN/n)
			C is some constant exploration factor
			N is the number of visits from the parent node
			n is the number of visits on the current node
		*/
		double UCB1(){
			//return if in the root node
			if(parent == NULL) return 0;
			int N = parent->getVisits();
			if(visits == 0){
				return DBL_MAX;	//return max value for unvisited Nodes
			}	
			double result = total_val/(double)visits + 
					C_CONST * sqrt( log(N) /(double)visits);
			return result;
		}
		//prints state and every child's state, as well as visits and total values
		void print(){
			cout<<"***********************"<<endl;
			cout<<"Total Value:\t"<<total_val<<endl;
			cout<<"Num Visits:\t"<<visits<<endl;
			cout<<"Avg Value:\t";
				if(visits == 0)	cout<<"Infinity"<<endl;
				else		cout<<total_val/(double)visits<<endl;
			cout<<"UCB1 SCORE:\t"<<UCB1()<<endl;
			cout<<"Heuristic:\t"<<state.heuristic()<<endl;
			state.print();
			if(children == NULL){
				cout<<"******NO CHILDREN******"<<endl;
				return;
			}
			cout<<"***PRINTING CHILDREN***"<<endl;
			for(int i = 0; i < 4; i++){
				if(!children[i].valid){
					cout<<"Skipped invalid child with move "<<map[i]<<endl;
					continue;
				}
				cout<<"MOVE: "<<map[i]<<endl;
				cout<<"Total Value:\t"<<children[i].total_val<<endl;
				cout<<"Num Visits:\t"<<children[i].visits<<endl;
				cout<<"Avg Value:\t";
					if(visits == 0)	cout<<"Infinity"<<endl;
					else 		cout<<total_val/(double)visits<<endl;
				cout<<"UCB1 Score: \t"<<children[i].UCB1()<<endl;
				cout<<"Heuristic:\t"<<children[i].state.heuristic()<<endl;
				children[i].state.print();
			}
		}
		//method that returns the number of nodes further in the tree, exclusing invalid ones
		int size(){
			int result = 0;
			if(!valid)	return 0;
			if(leaf)	return 1;
			for(int i = 0; i < 4; i++){
				result += children[i].size();
			}
			return 1 + result;
		}

		//this method contains and drives all four steps of the tree search
		void mcts(){

		//step one: finding a leaf node based off ucb1
			Node* current = this;
			//follow UCB1 down until a leaf node is reached
			while(!current->leaf){
				//the pick child method returns the index of the optimal 
				//child to follow using ucb1
				int index = current->pick_child();
				//updating current
				if(index < 0 || index > 3)	break;
				current  = &current->children[index];
			}
			//now current points to a leaf node, need to check if already visited
			if(current->visits != 0){
				//already visited, expand children and pick one

		//step three: expand (only sometimes, when leaf node is visited)
				current->expand();
				for(int i = 0; i < 4; i++){
					//choosing first valid child
					if(current->children[i].valid)
						break;
				}
			}
			//now current points to a valid leaf node ready for random walk

		//step three and first half of step four: random walk and backpropagate back to leaf
			double r_val = (random_walk(RANDOM_WALK_ITERATIONS, current->state)
				       /RANDOM_WALK_ITERATIONS);

			current->total_val += r_val;

		//step four finished: backpropagate values up the tree to the root
			while(current->parent != NULL){
				//increment visits
				current->visits++;
				//add on total value
				current->parent->total_val += current->total_val;
				//update current pointer
				current = current->parent;
			}
			//finally, update the number of visits at the root
			current->visits++;
			return;	
		}

		//simple method which selects a child based on maximum UCB1
		//for ties, the first tied child is picked
		int pick_child(){
			if(leaf)	return -1;
			double ucb1_score = 0;
                        double max_val = -1;
                        int max_index = -1;
                        for(int i = 0; i < 4; i++){
                                //only computing valid children
                                if(children[i].valid){
	                                //computing ucb1 for each child using current nodes visits
	                                ucb1_score = children[i].UCB1();
	                                //keeping track of maximum ucb1 and which child it was
	                                if(ucb1_score > max_val){
	                                        max_val = ucb1_score;
	                                        max_index = i;
	                                }
				}
                        }
			return max_index;
		}
		//method called on root to decide a move and delete rest of tree
		char pick_move(){
			if(leaf)	return 'Z';
			int index = pick_child();
			return map[index];
		}
};

int main(){
	bool display;
	int start[16];
	//input reading
	//first the puzzle
	for(int i = 0; i < 16; i++)
		cin>>start[i];
	//now reading y or not for display while searching
	char letter;
	cin>>letter;
	if(letter == 'y' || letter == 'Y')	display = true;
	else					display = false;
	//now checking if rand is seeded or not
	cin>>letter;
	if(letter == 'n' || letter == 'N')	srand(time(NULL));
	else					srand(letter);
	fifteen_puzzle p(start);
	//Node root;
	fifteen_puzzle game_board(p);
	/*	Main loop:
		Algorithm deliberates/updates values then makes a move
		stops when goal is reached
	*/
	int j = 0;
	while(!game_board.goal_test()){
		Node root(game_board);
		//loops through a set iteration of mcts before making a decision
		//number of iterations is subject to change
		for(int i = 0; i < MCTS_ITERATIONS; i++){
			root.mcts();
		}
		//after sufficiently exploring, the best child is chosen
		game_board.swap(root.pick_move());
		if(display){
			cout<<"MAIN LOOP ITERATION "<<j++<<endl;
			cout<<"DONE DELIBERATING, CHOSE TO MOVE "<<root.pick_move()<<endl;
			cout<<"PRINTING BOARD"<<endl;
			game_board.print();
		}
	}
	cout<<"GOAL FOUND"<<endl;
	
	return 0;
}
