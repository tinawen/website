#include <ilib.h>
#include <stdio.h>
#include <sys/archlib.h>
#include <errno.h>
#include <math.h>

#define maxDepth 30
#define xdimen 4
#define ydimen 4
#define connect 3
#define infinity 1000
typedef struct node {	
  char state[xdimen * ydimen];	
  int lastPos;
  struct node *next;	
} node;

typedef struct track{
	int position;
	int score;
} track;
char unoccupied = 0;
char white = 1;
char black = 2;				
char *map;

void display (char* map) {
	for (int j = (ydimen - 1); j >= 0; j--) {
		for (int i = 0; i < xdimen; i++) {	
			if (map[i + j*xdimen] == unoccupied) {
				printf("| ");
			}
			else if (map[i + j*xdimen] == white) {
				printf("|o");
			}
			else if (map[i + j*xdimen] == black) {
				printf("|x");
			}
		}
		printf("|\n");
	}
	printf("\n");
}
	
int spaceInColumn(int y, char* map) {
	for (int i = 0; i < ydimen; i++) {
		if (map[y + i*xdimen] == unoccupied) {
			return 1;
		}
	}
//	printf("column is full\n");
	return 0;
}
			
void update (char turn, int y, char* map) {
    for (int i = 0; i < ydimen; i++) {
        if (map[y + i*xdimen] == unoccupied) {
            map[y + i*xdimen] = turn;
            break;
        }
    }
    //display(map);
}

int countSpaces (char* currentMap) {
	int count = 0;
	for (int i = 0; i < (xdimen * ydimen); i++)  {
		if (currentMap[i] == unoccupied) {
			count++;
		}
	}
	return count;
}

int heuristic (char* currentMap, char turn) {
    int score = 0;
    int us;
    int them;
    for (int i = 0; i < ((ydimen - (connect - 1)) * xdimen); i++) {		//check n in a column
        us = 0;
        them = 0;
        for (int j = 0; j < connect; j++) {
            if (i+j*xdimen >= xdimen*ydimen) {
                printf("0 out of bound\n");
                return 0;
            }
            if (currentMap[i+j*xdimen] == turn) {
                us++;
            }
            else if (currentMap[i+j*xdimen] == (3 - turn)) {
                them++;
            }
        }
        if (us == connect) 
            return infinity + countSpaces(currentMap);
        else if (them == connect)
            return - infinity - countSpaces(currentMap);
        if ((us == 0) || (them == 0))
            score = score + (us*us) - (them*them);
    }
 
    for (int i = 0; i < (xdimen * ydimen - (connect - 1)); i++) {		//check n in a row
        us = 0;
        them = 0;
        if ((i % xdimen) < (xdimen - (connect - 1))) {
            for (int j = 0; j < connect; j++) {
                if (i+j >= xdimen*ydimen) {
                    printf("1 out of bound\n");
                    return 0;
                }
                if (currentMap[i+j] == turn) {
                    us++;
                }
                else if (currentMap[i+j] == (3 - turn)) {
                    them++;
                }
            }
            if (us == connect) 
                return infinity + countSpaces(currentMap);
            else if (them == connect)
                return - infinity - countSpaces(currentMap);
            if ((us == 0) || (them == 0))
                score = score + (us*us) - (them*them);
        }
    }
    
    for (int i = 0; i < ((ydimen - (connect - 1)) * xdimen - (connect - 1)); i++) {		//check n in a forward diagonal
        us = 0;
        them = 0;
        if ((i % xdimen) < (xdimen - (connect - 1))) {
            for (int j = 0; j < connect; j++) {
                if (i+j+j*xdimen >= xdimen*ydimen) {
                    printf("2 out of bound\n");
                    return 0;
                }
                if (currentMap[i+j+j*xdimen] == turn) {
                    us++; 
                }
                else if (currentMap[i+j+j*xdimen] == (3 - turn)) {
                    them++;
                }
            }
            if (us == connect) 
                return infinity + countSpaces(currentMap);
            else if (them == connect)
                return - infinity - countSpaces(currentMap);
            if ((us == 0) || (them == 0))
                score = score + (us*us) - (them*them);
        }
    }
    //   return 0;
    for (int i = (connect - 1); i < (ydimen - (connect - 1)) * xdimen; i++) {		//check n in a backward diagonal
        us = 0;
        them = 0;
        if ((i % xdimen) > (connect - 2)) {
            for (int j = 0; j < connect; j++) {
                if (i-j+j*xdimen >= xdimen*ydimen) {
                    printf("3 out of bound\n");
                    return 0;
                }
                if (currentMap[i-j+j*xdimen] == turn) {
                    us++; 
                }
                else if (currentMap[i-j+j*xdimen] == (3 - turn)) {
                    them++;
                }
            }
            if (us == connect) 
                return infinity + countSpaces(currentMap);
            else if (them == connect)
                return - infinity - countSpaces(currentMap);
            if ((us == 0) || (them == 0))
                score = score + (us*us) - (them*them);
        }
    }
    return score;
}
	 

node* generateMoves (char* currentMap, char turn) {
	node* first;
	first = (node*)malloc(sizeof(node));
	first->next = NULL;
	
	for (int i = 0; i < xdimen; i++) {
		if (spaceInColumn(i, currentMap) == 1) {
			node* newNode;
			newNode = (node*) malloc(sizeof(node));
			memcpy(newNode->state, currentMap, xdimen*ydimen*sizeof(char)); 
			update(turn, i, newNode->state);
			newNode->lastPos = i;
			newNode->next = first->next;
			first->next = newNode;
		}
	}
	/*
	//for debugging
	node* test = first;
	while (test->next != NULL) {
		display(test->next->state);
		printf("testing\n");
		test = test->next;
	}
	*/
	node* result = first->next;
	free(first);
	return result;
} 

track* getNextMove (char turn, char* map, int depth, int alpha, int beta) {
	track* t;
	t = (track*)malloc(sizeof(track));
	if (depth == 0) {
		t->score = heuristic(map, turn);
		t->position = -1;		//end
		return t;
	}
	
	if (abs(heuristic(map, turn)) >= infinity) { //game over
		t->score = heuristic(map, turn);
		t->position = -1;		//end
		return t;
	}
	
	t->score = -2 * infinity;
	node* possibleMoves = generateMoves(map, turn);
	if (possibleMoves == NULL) {		//game full -draw
		t->score = 0;
		t->position = -1;
		return t;
	}
	while (possibleMoves != NULL) {
		track* nextMove = getNextMove(3 - turn, possibleMoves->state, (depth - 1), -beta, -alpha);
		int newScore = nextMove->score;
		free(nextMove);
		if ((-newScore) >= beta) {
			t->score = (-newScore);
			t->position = -1;
			while (possibleMoves != NULL) {
				node* tmp = possibleMoves;
				possibleMoves = possibleMoves->next;
				free(tmp);
			}
			
			return t;
		}
		if ((-newScore) > t->score) {
			t->score = (-newScore);
			alpha = (-newScore);
			t->position = possibleMoves->lastPos;
			
		}
		node* tmp = possibleMoves;
		possibleMoves = possibleMoves->next;
	//	printf("freeing tmp\n");		
		free(tmp);
	//	printf("tmp freed\n");
	}
	return t;
}



int main(void)
{
	int start = get_cycle_count();			 //start timing
	map = (char*)malloc(sizeof(char) * xdimen * ydimen);
	bzero(map, xdimen * ydimen * sizeof(char));
	display(map);
	/*
	int input[5] = {1, 2, 1, 1, 0};
	for (int i = 0; i < 5; i++) {
		if (spaceInColumn(input[i], map) == 0) {
			printf("illegal move. The column is full\n");
		}
		else {
			update (black, input[i], map);
			display(map);
			if (heuristic_3 (map, black) == 1) {
				printf("the cross wins!\n");
				return 0;
			}
			else if (heuristic_3 (map, black) == -1) {
				printf("the circle wins!\n");
				return 0;
			}
			update(white, getNextMove(white, map, maxDepth)->position, map);
			display(map);
			if (heuristic_3 (map, white) == 1) {
				printf("the circle wins!\n");
				return 0;
			}
			else if (heuristic_3 (map, white) == -1) {
				printf("the cross wins!\n");
				return 0;
			}
		}
	}
	*/
	//play it against itself
	
	while (generateMoves(map, white) != NULL){
		update(white, getNextMove(white, map, maxDepth, -3*infinity, 3*infinity)->position, map);
		display(map);
		if (heuristic(map, white) >= infinity) {
			int end = get_cycle_count();			 //end timing
			printf("cycles took %d\n", end - start);
			printf("the circle wins!\n");
			return 0;
		}
		else if (heuristic(map, white) >= infinity) {
			int end = get_cycle_count();			 //end timing
			printf("cycles took %d\n", end - start);
			printf("the cross wins!\n");
			return 0;
		}
	
		if (generateMoves(map, black) != NULL){
			update(black, getNextMove(black, map, maxDepth, -3*infinity, 3*infinity)->position, map);
			display(map);
			if (heuristic(map, black) >= infinity) {
				int end = get_cycle_count();			 //end timing
				printf("cycles took %d\n", end - start);
				printf("the circle wins!\n");
				return 0;
			}
			else if (heuristic(map, black) >= infinity) {
				int end = get_cycle_count();			 //end timing
				printf("cycles took %d\n", end - start);
				printf("the cross wins!\n");
				return 0;
			}
		}
	}
	int end = get_cycle_count();			 //end timing
	printf("cycles took %d\n", end - start);
	//end
	printf("the game is a draw. =(\n");
    return 0;
}		

	
