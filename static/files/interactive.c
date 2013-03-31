#include <ilib.h>
#include <stdio.h>
#include <sys/archlib.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

#define P 50
#define maxDepth 8
#define xdimen 7
#define ydimen 6
#define connect 4
#define infinity 1000

typedef struct node{
    char state[xdimen * ydimen];
    int lastPos;
    struct node *parent;
    struct node *firstChild;
    struct node *next;
    int score;
    char ready;
} node;

typedef struct workElement{
    char workType;
    struct node* workNode;
    struct workElement* nextWork;
} workElement;

typedef struct workQueue{
    struct workElement* work;
}workQueue;

typedef struct sPacket{                   //send packets for each piece of work that is distributed from rank 0
    char state[xdimen * ydimen];
    int packet;
    char numMoves;
    char ready;
}sPacket;

typedef struct rPacket{                  //receive packets for storing results sending back to rank 0
    int score;
    int packet;
    char ready;
}rPacket;

char unoccupied = 0;
char white = 1;
char black = 2;				
char *map;
char combineW = 0;
char expandW = 1;
int packet;
char numMoves;
int numChildren;
int numIssuedWork;
node **trackNodes;
ilibStatus *status_out;
rPacket* returnPacket;
sPacket** sendPacketPointer;
sPacket* sendPacket;
rPacket* returnPacketPointer;
workQueue* q;
FILE *fptr;


node* createStartNode(char* map) {
    node* tn;
    tn = (node*) malloc(sizeof(node));
    memcpy(tn->state, map, xdimen*ydimen*sizeof(char)); 
    tn->lastPos = -1;
    tn->parent = NULL;
    tn->firstChild = NULL;
    tn->next = NULL;
    tn->ready = 0;
    tn->score = 0;
    return tn;
}

node* createNode (char* map, int lp, node* p, node* fc, node* n, char r, int s) {
    node* tn;
    tn = (node*) malloc(sizeof(node));
    memcpy(tn->state, map, xdimen*ydimen*sizeof(char)); 
    tn->lastPos = lp;
    tn->parent = p;
    tn->firstChild = fc;
    tn->next = n;
    tn->ready = r;
    tn->score = s;
    return tn;
}

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
    for (int j = 0; j < xdimen; j++) {
        printf("/%d", j);
    }
    printf("/\n");
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

void update (char turn, int y, char* map) {
    for (int i = 0; i < ydimen; i++) {
        if (map[y + i*xdimen] == unoccupied) {
            map[y + i*xdimen] = turn;
            break;
        }
    }
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

int countSpaces (char* currentMap) {
    return count(currentMap, unoccupied);
}

int heuristic (char* currentMap, char turn) {
    int score = 0;
    int us;
    int them;
    for (int i = 0; i < ((ydimen - (connect - 1)) * xdimen); i++) {		//check n in a column
        us = 0;
        them = 0;
        for (int j = 0; j < connect; j++) {
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

node* findChildren (char* currentMap, char turn, node* parent) {
    node* first;
    first = (node*)malloc(sizeof(node));
    first->next = NULL;
    
    for (int i = 0; i < xdimen; i++) {
        if (spaceInColumn(i, currentMap) == 1) {
            node* newNode;
            newNode = createNode(currentMap, i, parent, NULL, first->next, 0, 0);
            update(turn, i, newNode->state);
            first->next = newNode;
        }
    }
    node* result = first->next;
    free(first);
    return result;
} 

int checkChildrenReady (node* parent) {
    node* check;
    check = parent->firstChild;
    if (check == NULL) {
        return -1;
    }
    while (check != NULL) {
        if (check->ready == 0) {
            return 0;
        }
        check = check->next;
    } 
    return 1;
}

void addToQ (workQueue* q, char type, node* node) {
    workElement* newWork;
    newWork = (workElement*) malloc(sizeof(workElement));
    newWork->workType = type;
    newWork->workNode = node;
    newWork->nextWork = q->work;
    q->work = newWork;
}

char computeTurn (char* map) {
    if (count(map, white) == count(map, black)) {
        return white;
    }
    else {
        return black;
    }
}

int expandOthers (workQueue* q, node* tn, rPacket* returnPacketPointer) {
    int numChildren = 0;
    char turn = computeTurn(tn->state);
    int depth = xdimen * ydimen - countSpaces(tn->state) - numMoves;
    if ((depth >= maxDepth) || (countSpaces(tn->state) == 0) || abs((heuristic(tn->state, turn)) >= infinity)) {
        if (tn->parent == NULL) {     //ready to return
            rPacket returnPacket;
            returnPacket.score = heuristic(tn->state, turn);
            returnPacket.ready = 1;
            returnPacket.packet = packet;
            memcpy(returnPacketPointer, &returnPacket, sizeof(rPacket));
            return 0;
        }
        else {            
            tn->score = heuristic(tn->state, turn);
            tn->ready = 1;
            if (checkChildrenReady(tn->parent) == 1) {
                addToQ(q, combineW, tn->parent);
            }
            return 0;
        }
    }
    else {
        tn->firstChild = findChildren(tn->state, turn, tn);
        node* child = tn->firstChild;
        while (child != NULL) {
            numChildren++;
            addToQ(q, expandW, child);
            child = child->next;
        }
        return numChildren;
    }
}

int expand (workQueue* q, node* tn) {
    int numChildren = 0;
    char* map = tn->state;
    char turn = computeTurn(map);
    int depth = xdimen * ydimen - countSpaces(tn->state) - numMoves;
    if ((depth >= maxDepth) || (countSpaces(map) == 0) || abs((heuristic(map, turn)) >= infinity)) {
        tn->score = heuristic(map, turn);
        tn->ready = 1;
        if (checkChildrenReady(tn->parent) == 1) {
            addToQ(q, combineW, tn->parent);
        }
        return 0;
    }
    else {
        tn->firstChild = findChildren(map, turn, tn);
        node* child = tn->firstChild;
        while (child != NULL) {
            numChildren++;
	    addToQ(q, expandW, child);
            child = child->next;
        }
        return numChildren;
    }
}

void winnerCheck(char* map) {
    if (abs(heuristic(map, computeTurn(map))) >= infinity) {
        printf("winner found! The score for white is %d\n", heuristic(map, white));
        printf("The score for black is %d\n", heuristic(map, black));
        ilib_terminate();
        ilib_finish();
    }
    if (countSpaces(map) == 0) {
        printf("it's a draw\n");
        ilib_terminate();
        ilib_finish();
    }
}

void scorePrinting(node* tn) {
    tn->score = -(tn->firstChild->score);
    int nextPos = tn->firstChild->lastPos;
    node* child;
    child = tn->firstChild->next;
    free(tn->firstChild);
    while (child != NULL) {
        if (-(child->score) > tn->score) {
            tn->score = -(child->score);
            nextPos = child->lastPos;
        }
        node* tmp = child;
        child = child->next;
        free(tmp);
    }
    //   printf("score = %d nextPos = %d\n", tn->score, nextPos);
    update(computeTurn(tn->state), nextPos, tn->state);
    display(tn->state);
    winnerCheck(tn->state);
}


void send1Packet (workQueue* q, int i, sPacket** sendPacketPointer) {
    sPacket sendPacket;
    memcpy(&(sendPacket.state), &(q->work->workNode->state), xdimen*ydimen*sizeof(char));
    numIssuedWork++;
    sendPacket.packet = numIssuedWork;
    sendPacket.numMoves = numMoves;
    sendPacket.ready = 1;
    memcpy(sendPacketPointer[i-1], &sendPacket, sizeof(sPacket));
    trackNodes[numIssuedWork] = q->work->workNode;
    numChildren--;
    workElement* tmp = q->work;
    q->work = q->work->nextWork;
    free(tmp);
}

void sendPackets (workQueue* q, sPacket** sendPacketPointer) {
    for (int i = 1; i < P; i++) {
        send1Packet(q, i, sendPacketPointer);
    }
}

void regularCombine (workQueue* q, node* tn) {
    tn->score = -(tn->firstChild->score);
    node* child;
    child = tn->firstChild->next;
    free(tn->firstChild);
    while (child != NULL) {
        if (-(child->score) > tn->score) {
            tn->score = -(child->score);
        }
        node* tmp = child;
        child = child->next;
        free(tmp);
    }
    tn->ready = 1;
    if (checkChildrenReady(tn->parent) == 1) {
        addToQ(q, combineW, tn->parent);
    }
}

void combineSingle (workQueue* q, node* tn) {
    if (tn->parent == NULL) {
        scorePrinting(tn);
        int input;
        printf("It's your turn now. Please place your disc\n");
        while(1) {
            if (scanf("%d", &input) == 1) {
                if (spaceInColumn(input, tn->state) == 0) {
                    printf("Column %d you selected is full. Make a valid move!\n", input);
                }
                else if ((input >= xdimen) || (input <0)) {
                    printf("Your input %d is out of bound. Provide a valid input!\n", input);
                }
                else {
                    printf("You chose to place your disc in column %d\n", input);
                    update(computeTurn(tn->state), input, tn->state);
                    display(tn->state);
                    winnerCheck(tn->state);
                    numMoves++;
                    break;
                }
            }
            else {
                printf("You failed. Please input a number\n");
            }
        }
	printf("My turn now. Thinking\n");
        numMoves++;
        q->work = NULL;
        addToQ(q, expandW, createStartNode(tn->state));
    }
    else {
        regularCombine(q, tn);
    }
}

void getWorkSingle(workQueue* q) {
    if (q->work != NULL) {
        workElement* we = q->work;
        q->work = q->work->nextWork;
        if (we->workType == expandW) {
            expand(q, we->workNode);
        }
        else if (we->workType == combineW) {
            combineSingle(q, we->workNode);
        }
        free(we);
    }
}


void combineFor0 (workQueue* q, node* tn, sPacket** sendPacketPointer) {
    if (tn->parent == NULL) {
	scorePrinting(tn);
        int input;
        printf("It's your turn now. Please place your disc\n");
        while (1) {
            if (scanf("%d", &input) == 1) {
                if (spaceInColumn(input, tn->state) == 0) {
                    printf("Column %d you selected is full. Make a valid move!\n", input);
                }
                else if ((input >= xdimen) || (input <0)) {
                    printf("Your input %d is out of bound. Provide a valid input!\n", input);
                }
                else {
                    printf("You chose to place your disc in column %d\n", input);
                    update(computeTurn(tn->state), input, tn->state);
                    display(tn->state);
                    winnerCheck(tn->state);
                    numMoves++;
                    break;
                }
            }
             else {
                printf("You failed. Please input a number\n");
             }
        }
        printf("My turn now. Thinking...\n");
	numMoves++;
	numChildren = 1;
	numIssuedWork = 0;
        int skip = 0;
        trackNodes[0] = createStartNode(tn->state);
        free(tn);
        q->work = NULL;
        if (numChildren <= (P-1)) {
            numChildren--;
            numChildren += expand(q, trackNodes[0]);
            if (numChildren == 1) {
                addToQ(q, expandW, trackNodes[0]);
                while(1) {
                    getWorkSingle(q);
                }
            }    
	    if (numChildren >= (P-1)) {
		sendPackets(q, sendPacketPointer);  
                skip = 1;
            }
        }        
	while ((numChildren <= (P-1)) && (skip == 0)) {
            numChildren--;
            workElement *last = q->work;
            workElement *beforeLast;
            while (last->nextWork != NULL) {
                beforeLast = last;
                last = last->nextWork;
            }
            node* node2expand = last->workNode;
            beforeLast->nextWork = NULL;
            int e = expand(q, node2expand);
            if (e == 0) {
                for (int i = 1; i <= (numChildren + 1); i++) {
                    send1Packet(q, i, sendPacketPointer);
                }
                return;
            }
	    else {
                numChildren += e;
            }
	    //  if (numChildren == 1) {
	    //     addToQ(q, expandW, trackNodes[0]);
            //    while(1) {
            //        getWorkSingle(q);
	    //     }
	    //  }    
	    if (numChildren >= (P-1)) {
		sendPackets(q, sendPacketPointer);
                return;
	    }
	}
    }
    else {
	regularCombine(q, tn);
    }
}


void combine (workQueue* q, node* tn, rPacket* returnPacketPointer) {
    if (tn->parent == NULL) {
	rPacket returnPacket;
	returnPacket.score = -(tn->firstChild->score);
	node* child;
	child = tn->firstChild->next;
        free(tn->firstChild);
	while (child != NULL) {
	    if (-(child->score) > returnPacket.score) {
		returnPacket.score = -(child->score);
	    }
	    node* tmp = child;
	    child = child->next;
            free(tmp);
	}
        free(tn);
        returnPacket.ready = 1;
	returnPacket.packet = packet;
     	memcpy(returnPacketPointer, &returnPacket, sizeof(rPacket));
    }
    else {
        regularCombine(q, tn);
    }
}
	
void getWork(workQueue* q) {
    if (q->work != NULL) {
        workElement* we = q->work;
        q->work = q->work->nextWork;
        if (we->workType == expandW) {
            expand(q, we->workNode);
        }
        else if (we->workType == combineW) {
	    combineSingle(q, we->workNode);
	}
        free(we);
    }
}

void getWorkOthers(workQueue* q, rPacket* returnPacketPointer) {
    if (q->work != NULL) {
        workElement* we = q->work;
        q->work = q->work->nextWork;
        if (we->workType == expandW) {
            expandOthers(q, we->workNode, returnPacketPointer);
        }
        else if (we->workType == combineW) {
	    combine(q, we->workNode, returnPacketPointer);
        }
        free(we);
    }
}
 
void getWorkFor0(workQueue* q, sPacket** sendPacketPointer) {
    if (q->work != NULL) {
        workElement* we = q->work;
        q->work = q->work->nextWork;
        if (we->workType == expandW) {
            expand(q, we->workNode);
        }
        else if (we->workType == combineW) {
            combineFor0(q, we->workNode, sendPacketPointer);
        }
        free(we);
    }
}

int main(void)
{
    ilib_init();
    fptr = fopen("/dev/tty", "r");
    if (ilib_proc_go_parallel(P, NULL) != ILIB_SUCCESS)
        ilib_die("Falled to go parallel.");
    int rank = ilib_group_rank(ILIB_GROUP_SIBLINGS);
    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
    
    if (rank == 0) {
       returnPacket = (rPacket *) malloc_shared((P-1) * sizeof(rPacket));
	for (int i = 0; i < P; i++) {
	    returnPacket[i].ready = 0;
	}
        sendPacketPointer = (sPacket **) malloc((P-1) * sizeof(sPacket *));
	status_out = (ilibStatus *) malloc((P-1) * sizeof(ilibStatus));
        for (int i = 1; i < P; i++) {
	    rPacket* data2send = &(returnPacket[i-1]);
            ilib_msg_send(ILIB_GROUP_SIBLINGS, i, i, &data2send, sizeof(rPacket*));
	    ilib_msg_receive(ILIB_GROUP_SIBLINGS, i, i+P, &(sendPacketPointer[i-1]), sizeof(sPacket*), &(status_out[i-1]));
        }
    }
    else {
        status_out = (ilibStatus *) malloc(sizeof(ilibStatus));
        sendPacket = (sPacket *) malloc_shared(sizeof(sPacket));
	sendPacket->ready = 0;
	returnPacketPointer = (rPacket *) malloc(sizeof(rPacket));
        ilib_msg_receive(ILIB_GROUP_SIBLINGS, 0, rank, &(returnPacketPointer), sizeof(rPacket*), status_out);
        ilib_msg_send(ILIB_GROUP_SIBLINGS, 0, rank + P, &sendPacket, sizeof(sPacket*));
    }

    ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
      
    if (rank == 0) {
        int skip = 0;
        int order;
        char *map;
        map = (char*)malloc(sizeof(char) * xdimen * ydimen);
        bzero(map, xdimen * ydimen * sizeof(char));
        display(map);
        numMoves = 0;
        printf("Welcome to the Connect-4 game! If you want to play first, key '0'; If you want me to play first, key '1'. Press ENTER to confirm\n");
        while(1) {            
            if (scanf("%d", &order) == 1) {
                if (order == 0) { 
                    printf("You chose to play first. You are circle. Please key in the column number you want to place your disc. Column numbers are shown below the grid.\n");                
                    break;
                }
                else if (order == 1) {
                    printf("You want me to play first. You are cross. Good luck!\n");
                    break;
                }
                else
                    printf("Stop being dumb. You can only key 0 or 1\n");
            }
             else {
                printf("You failed. Please input a number\n");
             }
        }
        if (order == 0) {
            int input;
            printf("It's your turn now. Please place your disc\n");
            while (1) {
                if (scanf("%d", &input) == 1) {
                    if ((input >= xdimen) || (input <0)) {
                        printf("Your input %d is out of bound. Provide a valid input!\n", input);
                    }
                    else if ((input >= xdimen) || (input <0)) {
                        printf("Your input %d is out of bound. Provide a valid input!\n", input);
                    }
                    else {
                        printf("You chose to place your disc in column %d\n", input);
                        update(computeTurn(map), input, map);
                        display(map);
                        numMoves++;
                        break;
                    }
                }
                 else {
                     printf("You failed. Please input a number\n");
                 }
            }
        }
        printf("My turn now. Thinking...\n");
        q = (workQueue*) malloc(sizeof(workQueue));
        q->work = NULL;
        numChildren = 1;
        numIssuedWork = 0;
        node* startNode = createStartNode(map);
        free(map);
        if (numChildren <= (P-1)) {
            numChildren--;
            numChildren += expand(q, startNode);
	    //   if (numChildren == 1) {
	    //     addToQ(q, expandW, startNode);
	    //     while(1)
            //        getWork(q);
	    //   }
            if (numChildren >= (P-1)) {
                trackNodes = (node **) malloc(2 * (numChildren + 1) * sizeof(node *));
                trackNodes[0] = startNode;
                sendPackets(q, sendPacketPointer);
                skip = 1;
            }
        }
        while ((numChildren <= (P-1)) && (skip == 0)) {
            numChildren--;
            workElement *last = q->work;
            workElement *beforeLast;
            while (last->nextWork != NULL) {
                beforeLast = last;
                last = last->nextWork;
            }
            node* node2expand = last->workNode;
            beforeLast->nextWork = NULL;
            int e = expand(q, node2expand);
            if (e == 0) {
                for (int i = 1; i <= (numChildren + 1); i++) {
                    send1Packet(q, i, sendPacketPointer);
                }
                break;
            }
            else {
                numChildren += e;
            }
            if (numChildren >= (P-1)) {
                trackNodes = (node **) malloc(2 * (numChildren + 1) * sizeof(node *));
                trackNodes[0] = startNode;
                sendPackets(q, sendPacketPointer);
                break;
            }
        }
        
        while(1) {
            for (int i = 1; i < P; i++) {
                if (returnPacket[i-1].ready == 1) {
                    returnPacket[i-1].ready = 0;
                    packet = returnPacket[i-1].packet;
                    trackNodes[packet]->score = returnPacket[i-1].score;
                    trackNodes[packet]->ready = 1;
                    if (checkChildrenReady(trackNodes[packet]->parent) == 1) {
                        addToQ(q, combineW, trackNodes[packet]->parent);
                    }
                    if (numChildren > 0) {
                        send1Packet(q, i, sendPacketPointer);
                    }
                }
            }
            
            if (q->work != NULL) {
                while (q->work->workType == combineW) {
                    getWorkFor0(q, sendPacketPointer);
                    if (q->work == NULL) 
                        break;
                }
            }
        }
    }
    else {
        q = (workQueue*) malloc(sizeof(workQueue));
        while (1) {
            if (sendPacket->ready ==1) {
                sendPacket->ready = 0;
                packet = sendPacket->packet;
		numMoves = sendPacket->numMoves;
		addToQ(q, expandW, createStartNode(sendPacket->state));
                while (q->work != NULL) {
		    getWorkOthers(q, returnPacketPointer);
		}
	    }
	}
    }
    ilib_finish();
    return 0;
}		
        
