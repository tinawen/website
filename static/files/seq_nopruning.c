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
    return 0;
}

int count (char* currentMap, char color) {
    int count = 0;
    for (int i = 0; i < (xdimen * ydimen); i++)  {
        if (currentMap[i] == color) {
            count++;
        }
    }
    return count;
}

char computeTurn (char* map) {
    if (count(map, white) == count(map, black)) {
        return white;
    }
    else {
        return black;
    }
}

void update (char turn, int y, char* map) {
    for (int i = 0; i < ydimen; i++) {
        if (map[y + i*xdimen] == unoccupied) {
            map[y + i*xdimen] = turn;
            break;
        }
    }
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
            if (i+j*xdimen >= xdimen*ydimen)
                printf("out of bound\n");
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
                if (i+j >= xdimen*ydimen)
                    printf("out of bound\n");
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
                if (i+j+j*xdimen >= xdimen*ydimen)
                    printf("out of bound\n");
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

    for (int i = (connect - 1); i < (ydimen - (connect - 1)) * xdimen; i++) {		//check n in a backward diagonal
        us = 0;
        them = 0;
        if ((i % xdimen) > (connect - 2)) {
            for (int j = 0; j < connect; j++) {
                if (i-j+j*xdimen >= xdimen*ydimen)
                    printf("out of bound\n");
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
    return first->next;
} 

track* getNextMove (char turn, char* map, int depth) {
    track* t;
    t = (track*)malloc(sizeof(track));
    if (depth == 0) {
        t->score = heuristic(map, turn);
        t->position = -1;		//end
        return t;
    }
    
    if (abs(heuristic(map, turn)) >= infinity) {
        t->score = heuristic(map, turn);
        t->position = -1;		//end
        return t;
    }
    
    t->score = -2*infinity;
    node* possibleMoves = generateMoves(map, turn);
    if (possibleMoves == NULL) {
        t->score = 0;
        t->position = -1;
        return t;
    }
    while (possibleMoves != NULL) {
        int newScore = (getNextMove(3 - turn, possibleMoves->state, (depth - 1)))->score;
        if ((-newScore) > t->score) {
            t->score = (-newScore);
            t->position = possibleMoves->lastPos;
        }
        possibleMoves = possibleMoves->next;
    }
    return t;
}



int main(void)
{
    unsigned long int start = get_cycle_count();			 //start timing
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
    
    while (generateMoves(map, computeTurn(map)) != NULL){
        int nextPos = getNextMove(computeTurn(map), map, maxDepth)->position; 
        update(computeTurn(map), nextPos, map);
        display(map);
        if (abs(heuristic(map, computeTurn(map))) >= infinity) {
            printf("winner found! The score for white is %d\n", heuristic(map, white));
            printf("The score for black is %d\n", heuristic(map, black));
            unsigned long int end = get_cycle_count();			 //end timing
            printf("cycles took %ul\n", end - start);
            ilib_terminate();
            ilib_finish();
        }
        if (countSpaces(map) == 0) {
            printf("it's a draw\n");
            unsigned long int end = get_cycle_count();			 //end timing
            printf("cycles took %ul\n", end - start);
            ilib_terminate();
            ilib_finish();
        }
        else 
            printf("score = %d nextPos = %d\n", heuristic(map, computeTurn(map)), nextPos);
    }
}
            
            

	
