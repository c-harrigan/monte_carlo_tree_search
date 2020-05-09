
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

#define RANDOM_WALK_ITERATIONS 5000
#define MCTS_ITERATIONS 1000
#define C_CONST 2.0

using namespace std;
//global variable to map the four integers to their actions
//U is 0, D is 1, L is 2, and R is 3
//const char map[4] = {'U', 'D', 'L', 'R'};

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
//static int i = 0;
//cout<<"IN RANDOM WALK "<<i++<<endl;
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
		Node* children;
		//tracking number of times a node is visited (updated during backpropagation)
		int visits;
		//tracking total value of a node (updated during backpropagation)
		double total_val;
		//parent pointer
		Node* parent;
		//puzzle held within each node representing the state
		fifteen_puzzle state;
		bool valid;

/*		//private constructor used most often. called during node expandion
		Node(Node* par, fifteen_puzzle p){
//TODO be careful with passing puzzles around local space, may need a copy() method call
			parent = par;
			state = p;
			children = NULL;
			total_val = 0;
			visits = 0;
		}
*/		
	public:
		//method to deep delete all nodes further down in the tree
		void clear_children(){
			if(children == NULL)	return;
			for(int i = 0; i < 4; i++){
				children[i].clear_children();
//				state->clear();
			}
			//delete children;
		}
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
		Node(Node* par, fifteen_puzzle p){
			state.copy(p);
			total_val = state.heuristic();
			visits = 0;
			parent = par;
			valid = true;
		}

		//basic constructor, probably unused
		Node(){
			children = NULL;
			visits = 0;
			total_val = 0;
			parent = NULL;
			state = NULL;
			valid = false;
		}
		//constructor for root node, given a starting fifteen puzzle
		Node(fifteen_puzzle p){
			children = NULL;
			visits = 0;
			total_val = p.heuristic();
			parent = NULL;
			state.copy(p);
			valid = true;
		}
		Node operator=(const Node& n){
			children = NULL;
			visits = 0;
			total_val = n.state.heuristic();
			state.copy(n.state);
			parent = NULL;
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
				if(!state.valid_swap(map[i]))	children[i] = Node();
				children[i] = Node(this, fifteen_puzzle(state, map[i]));
			}
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
/*
if(result > 1000){
cout<<"UCBI LARGE, DUMPING INFO AND EXITING"<<endl;
cout<<"N = "<<N<<". Visits = "<<visits<<". total_val = "<<
total_val<<". V = "<<total_val/(double)visits<<". C*sqrt(log(N)/(double)visits) = "<<
C*sqrt(log(N)/(double)visits)<<endl;//exit(0);
}*/
//cout<<"UCB1 RESULT: "<<result<<endl;
			return result;
		}
		//prints state and every child's state, as well as visits and total values
		void print(){
			cout<<"***********************"<<endl;
			cout<<"Total Value:\t"<<total_val<<endl;
			cout<<"Num Visits:\t"<<visits<<endl;
			cout<<"UCB1 SCORE:\t"<<UCB1()<<endl;
			cout<<"Heuristic:\t"<<state.heuristic()<<endl;
			state.print();
			if(children == NULL){
				cout<<"******NO CHILDREN******"<<endl;
				return;
			}
			cout<<"***PRINTING CHILDREN***"<<endl;
			for(int i = 0; i < 4; i++){
				if(!children[i].valid)	continue;
				cout<<"MOVE: "<<map[i]<<endl;
				cout<<"Total Value:\t"<<children[i].total_val<<endl;
				cout<<"Num Visits:\t"<<children[i].visits<<endl;
				cout<<"UCB1 Score: \t"<<children[i].UCB1()<<endl;
				cout<<"Heuristic:\t"<<children[i].state.heuristic()<<endl;
				children[i].state.print();
			}
/*			for(int i = 0; i < 4; i++){
				if(children[i] == NULL)	continue;
				children[i]->print();
			}
*/
		}

		//this method contains and drives all four steps of the tree search
		void mcts(){
static int j = 0;
cout<<j++<<" ";
if(!j%70)cout<<endl;
		//step one: finding a leaf node based off ucb1
			Node* current = this;
//cout<<1<<endl;
			//follow UCB1 down until a leaf node is reached
			while(current->children != NULL){
				//the pick child method returns the index of the optimal 
				//child to follow using ucb1
//cout<<2<<endl;
				int index = current->pick_child();
				//updating current
				if(index < 0 || index > 3)	break;
				current  = &current->children[index];
//cout<<4<<endl;
			}
//cout<<5<<endl;
			//now current points to a leaf node, need to check if already visited
			if(current->visits != 0){
				//already visited, expand children and pick one
//cout<<6<<endl;
		//step three: expand (only sometimes, when leaf node is visited)
				current->expand();
				for(int i = 0; i < 4; i++)
					//choosing first valid child
					if(current->children[i].valid)
						break;
			}
			//now current points to a valid leaf node ready for random walk

		//step three and first half of step four: random walk and backpropagate back to leaf
			double r_val = random_walk(RANDOM_WALK_ITERATIONS, current->state);
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
			if(children == NULL)	return -1;
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
		void pick_move(){
			if(children == NULL)	return;
			int index = pick_child();
			state.swap(map[index]);
			visits = 0;
			total_val = state.heuristic();
		//	for(int i = 0; i < 4; i++){
		//		if(children[i] == NULL)	continue;
		//		children[i]->clear();
		//	}
			clear_children();
			children = NULL;
		}
};

int main(){
	srand(time(NULL));
	int start[16] = {5,4,3,2,1,6,7,8,9,0,10,11,12,13,14,15};
	fifteen_puzzle p(start);
	Node root(p);
	Node temp;
	/*	Main loop:
		Algorithm deliberates/updates values then makes a move
		stops when goal is reached
	*/
int i = 0;
	while(!root.is_goal()){
cout<<"MAIN LOOP ITERATION "<<i++<<endl;
		//loops through a set iteration of mcts before making a decision
		//number of iterations is subject to change
		for(int i = 0; i < MCTS_ITERATIONS; i++){
			root.mcts();
		}
cout<<"DONE DELIBERATING, PRINTING"<<endl;
root.getState().print();
//root.print();
		//after sufficiently exploring, the best child is chosen
		root.pick_move();
//exit(0);
	}
	cout<<"GOAL FOUND"<<endl;
	
	return 0;
}
