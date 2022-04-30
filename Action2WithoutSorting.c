//Action 2

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>   
#include <math.h>
#include <semaphore.h>

typedef struct Node{
    int value;
    struct Node * prevNode; // stores previous node for linked list
    struct Node * nextNode; // stores next node for linked list

    int highestPrimeNumber; //For numbers that are not prime numbers, this field will indicate the highest prime number
                            //that has been processed by a given thread that is less than the given number
    int numPrimeNumbers;    //For numbers that are not prime numbers, this field will indicate the number of prime numbers that has been 
                            // processed by a thread before this number
    int threadID;           // Indicates the thread that identified this number;
    int teamNumber;         // Indicates the team that identified this number;
    int threadNumber;       // Indicates the number of the thread on this team that identified this number;
    int index;              // index at which this number was found
    clock_t begin;
    double elapsed;
    clock_t end;
} Node;

typedef struct Subset{
    int start;
    int end;
    int teamNumber;
    int threadNumber;
    clock_t begin;
    double elapsed;
    clock_t finishTime;
} Subset;

typedef struct Queue{
    Node * front;
    Node * last; //stores last node in queue 
} Queue;

Queue * threadFinishTimes;
Queue * teamFinishTimes;

typedef struct BulletinBoard{
    int primeNumbersIdentified; // Total Number of Prime Numbers Identified in the Bulletin Board(i.e. Length of "primeNumbersFound" linked list)

    Node * uniquePrimeNumbers;  //Linked List of Unique Prime Numbers that Have Been Identified
    Node * primeNumbersFound;   // Linked List of Prime Numbers That Have Been Identified

    Node * nonPrimeNumbersFound;    // Linked List of Non-Prime Numbers That Have Been Identified
} BulletinBoard;
BulletinBoard * PrimeNumbersStorage;


int NUM_TEAMS;
int NUM_THREADS_PER_TEAM;
int * ARRAY;
int arraySize;

pthread_t* threadIDs;
pthread_attr_t* threadAttributes;

pthread_t* teamIDs;
pthread_attr_t* teamAttributes;

pthread_mutex_t mutexVariable = PTHREAD_MUTEX_INITIALIZER;
sem_t queueMutexVariable;
sem_t teamQueueMutexVariable;
pthread_cond_t conditionVariable = PTHREAD_COND_INITIALIZER;

int readingFromBulletin = 0;
int writingToBulletin = 0;
int writers = 0;

Subset ** teamData;

Subset ** threadData;

int * HashTable;

char * filename;
FILE * fptr;

void addNodeToQueue(Node * node, Queue * queue){
    Node * front = queue->front;
    Node * last = queue->last;
    if(last == NULL){
        queue -> last = node;
        queue -> front = node;
    }
    else{
        queue->last -> nextNode = node;
        queue -> last = node;
    }
}
void addUniquePrimeToBulletinBoard(int num){
    Node * listFront = PrimeNumbersStorage -> uniquePrimeNumbers;
    Node * newNode = (Node *)(malloc(sizeof(Node)));
    newNode -> value = num;
    newNode -> nextNode = listFront;
    PrimeNumbersStorage -> uniquePrimeNumbers = newNode;
    HashTable[num] = 1;
}

void addThreadDataToQueue(Subset * data){
    sem_wait(&queueMutexVariable);
    Node * newNode = (Node *)(malloc(sizeof(Node)));
    newNode -> threadID = (int)(pthread_self());
    newNode -> teamNumber = data -> teamNumber;
    newNode -> threadNumber = data -> threadNumber;
    newNode -> begin = data -> begin;
    newNode -> end = data -> finishTime;
    newNode -> elapsed = data -> elapsed;
    addNodeToQueue(newNode, threadFinishTimes);
    sem_post(&queueMutexVariable);
}

void addTeamDataToQueue(Subset * data){
    sem_wait(&teamQueueMutexVariable);
    Node * newNode = (Node *)(malloc(sizeof(Node)));
    newNode -> threadID = (int)(pthread_self());
    newNode -> teamNumber = data -> teamNumber;
    newNode -> threadNumber = data -> threadNumber;
    newNode -> begin = data -> begin;
    newNode -> end = data -> finishTime;
    newNode -> elapsed = data -> elapsed;
    addNodeToQueue(newNode, teamFinishTimes);
    sem_post(&teamQueueMutexVariable);
}

void addPrimeToBulletinBoard(int num, Subset * data, int counter){
    Node * listFront = PrimeNumbersStorage -> primeNumbersFound;
    Node * ptr = listFront;
    
    Node * newNode = (Node *)(malloc(sizeof(Node)));
    newNode -> value = num;
    newNode -> threadID = (int)(pthread_self());
    newNode -> teamNumber = data -> teamNumber;
    newNode -> threadNumber = data -> threadNumber;
    newNode -> index = counter;

    newNode -> nextNode = listFront;
    PrimeNumbersStorage -> primeNumbersFound = newNode;
    
    PrimeNumbersStorage -> primeNumbersIdentified ++;
    int alreadyPresent = HashTable[num];
    if(alreadyPresent != 1){
        addUniquePrimeToBulletinBoard(num);
    }
}

void addNonPrimeNumberToBulletinBoard(int num, int numProcessed, int highestPrime, Subset * data, int counter){
    Node * listFront = PrimeNumbersStorage -> nonPrimeNumbersFound;
    Node * ptr = listFront;
   
    Node * newNode = (Node *)(malloc(sizeof(Node)));
    newNode -> value = num;
    newNode -> numPrimeNumbers = numProcessed;
    newNode -> highestPrimeNumber = highestPrime;
    newNode -> threadID = (int)(pthread_self());
    newNode -> teamNumber = data -> teamNumber;
    newNode -> threadNumber = data -> threadNumber;
    newNode -> index = counter;
    
    newNode -> nextNode = ptr;  
    PrimeNumbersStorage -> nonPrimeNumbersFound = newNode;
}

int isPrimeNumberInBulletinBoard(int num, Subset * data, int counter){
    if(PrimeNumbersStorage -> primeNumbersIdentified == 0){
        return 0;
    }
    int alreadyPresent = 0;
    Node * listFront = PrimeNumbersStorage -> primeNumbersFound;
    alreadyPresent = HashTable[num];
    if(alreadyPresent == 1){
        addPrimeToBulletinBoard(num, data, counter);
    }
    return alreadyPresent;
}

int maximum(int a , int b){
    if(a < b){
        return b;
    }
    return a;
}

int isPrime(int number){
    int counter = 2;
    int prime = 1;
    int root = (int)(sqrt(number));
    for(; counter <= root; counter ++){
        if(number % counter == 0){
            prime = 0;
            break;
        }
    }
    return prime;
}

void * threadFunction(void * arg){
    clock_t begin = clock() / CLOCKS_PER_SEC;
    Subset * ptr = (Subset*)(arg);
    ptr -> begin = begin;
    fprintf(fptr, "(Team Member) Hi My name is Thread %d and I am a member of team %d and My Number is %d\n", (int)(pthread_self()), ptr -> teamNumber, ptr -> threadNumber);
    int start = ptr -> start;
    int end = ptr -> end;
    int primesProcessed = 0;
    int highestPrimeProcessed = -1;

    for(int counter = start; counter <= end; counter ++){
        int numToAnalyze = *(ARRAY + counter);
        int prime;

        //-----SYNCHRONIZATION USING READERS/WRITERS CONCEPT-----////

        ///------ READERS CODE------///

        pthread_mutex_lock(&mutexVariable);
        if(writers == 1){
            pthread_cond_wait(&conditionVariable, &mutexVariable);
        }
        while(writingToBulletin == 1){
            pthread_cond_wait(&conditionVariable, &mutexVariable);
        }
        readingFromBulletin = 1;
        pthread_mutex_unlock(&mutexVariable);

        //fprintf(fptr, "Thread %d is going to read from the bulletin board now \n", (int)(pthread_self()));
        int alreadyPresent = isPrimeNumberInBulletinBoard(numToAnalyze, ptr, counter);
        if(alreadyPresent == 1){
            prime = 1;
        }
        else{
            prime = isPrime(numToAnalyze);
        }


        pthread_mutex_lock(&mutexVariable);
        readingFromBulletin = 0;
        pthread_cond_broadcast(&conditionVariable);
        pthread_mutex_unlock(&mutexVariable);

        ///------ WRITERS CODE------///

        pthread_mutex_lock(&mutexVariable);
        writers = 1;
        while(writingToBulletin == 1 || readingFromBulletin == 1){
            pthread_cond_wait(&conditionVariable, &mutexVariable);
        }
        writingToBulletin = 1;
        pthread_mutex_unlock(&mutexVariable);

        //fprintf(fptr, "Thread %d is going to write to the bulletin board now \n", (int)(pthread_self()));

        if(prime == 1 && alreadyPresent == 0){
            addPrimeToBulletinBoard(numToAnalyze, ptr, counter);
            primesProcessed ++;
            highestPrimeProcessed = maximum(highestPrimeProcessed, numToAnalyze);
        }
        else if(prime == 0 && alreadyPresent == 0){
            addNonPrimeNumberToBulletinBoard(numToAnalyze, primesProcessed, highestPrimeProcessed, ptr, counter);
        }

    
        pthread_mutex_lock(&mutexVariable);
        writers = 0;
        writingToBulletin = 0;
        pthread_cond_broadcast(&conditionVariable);
        pthread_mutex_unlock(&mutexVariable);
    }
    clock_t finishTime = clock() / CLOCKS_PER_SEC;
    double elapsed = ((double)(finishTime - begin)) / CLOCKS_PER_SEC;
    ptr -> elapsed = elapsed;
    ptr -> finishTime = finishTime;
    addThreadDataToQueue(ptr);
    pthread_exit(NULL);
    return NULL;
}

int generateRandomOddNumberArray(){
    srand ( time(NULL) );
    //arraySize = (rand() % 90001) + 10000;
    arraySize = 100000;
    ARRAY = malloc(arraySize * sizeof(int));
    for(int i = 0; i < arraySize; i ++){
        ARRAY[i] = (rand() % 49001) + 1000;
    }
    return 0;
}


/**
 * @brief This function was used purely for testing purposes
 * 
 */
void displayArray(){
    for(int i = 0; i < arraySize; i ++){
        if(i < arraySize - 1){
            fprintf(fptr, "%d, ", ARRAY[i]);
        }
        else{
            fprintf(fptr, "%d", ARRAY[i]);
        }
        
    }
    fprintf(fptr, "\n");
}

void displayThreadFinishTimes(){
    fprintf(fptr, "-----------------THREAD FINISH TIMES-----------------\n");
    Node * front = threadFinishTimes->front;
    Node * ptr = front;
    while (ptr != NULL){
        char * str = "ThreadID: %d, teamNumber: %d, threadNumber: %d \n";
        fprintf(fptr, str , ptr -> threadID, ptr -> teamNumber, ptr -> threadNumber);
        ptr = ptr -> nextNode;
    }
    fprintf(fptr, "\n");
}

void displayTeamFinishTimes(){
    fprintf(fptr, "-----------------TEAM FINISH TIMES-----------------\n");
    Node * front = teamFinishTimes->front;
    Node * ptr = front;
    while (ptr != NULL){
        char * str = " teamNumber: %d\n";
        fprintf(fptr, str , ptr -> teamNumber);
        ptr = ptr -> nextNode;
    }
    fprintf(fptr, "\n");
}

/**
 * @brief This function was used for debugging purposes after all threads have finished
 * writing to bulletin board
 * 
 */

void displayBulletinBoard(){
    fprintf(fptr, "--------DISPLAYING ARRAY ---------\n");
    displayArray();

    fprintf(fptr, "--------DISPLAYING UNIQUE PRIME NUMBERS FOUND ---------\n");
    Node * listFront = PrimeNumbersStorage -> uniquePrimeNumbers;
    Node * ptr = listFront;
    char * str = "%d, ";
    while(ptr != NULL){
        if(ptr -> nextNode == NULL){
            fprintf(fptr,"%d", ptr ->value);
        }
        else{
            fprintf(fptr,str, ptr ->value);
        }
        ptr = ptr -> nextNode;
    }
    fprintf(fptr, "\n\n-------Prime Numbers Identified--------\n");
    listFront = PrimeNumbersStorage -> primeNumbersFound;
    ptr = listFront;
    str = "Value: %d, Located At Index: %d, Identified By Team: %d, Identified by Thread: %d, ThreadID that Identified this value: %d\n";
    while(ptr != NULL){
        fprintf(fptr,str, ptr ->value, ptr -> index, ptr ->teamNumber, ptr -> threadNumber, ptr -> threadID,  ptr -> highestPrimeNumber, ptr -> numPrimeNumbers);
        ptr = ptr -> nextNode;
    }

    fprintf(fptr,"\n\n-------Non-Prime Numbers Identified -------\n");
    listFront = PrimeNumbersStorage -> nonPrimeNumbersFound;
    ptr = listFront;
    str = "Value: %d, Located At Index: %d, Identified By Team: %d, Identified by Thread: %d, ThreadID that Identified this value: %d, Highest Prime Identified Before: %d, Num Primes Before: %d\n";
    while(ptr != NULL){
        fprintf(fptr,str, ptr ->value, ptr -> index,  ptr ->teamNumber, ptr -> threadNumber, ptr -> threadID,  ptr -> highestPrimeNumber, ptr -> numPrimeNumbers);
        ptr = ptr -> nextNode;
    }
    fprintf(fptr, "\n");
}


void * teamFunction(void * arg){
    clock_t begin = clock() / CLOCKS_PER_SEC;
    Subset * teamMemberInformation = NULL;
    Subset * ptr = (Subset*)(arg);
    ptr -> begin = begin;
    int subArraySize = ptr -> end - ptr -> start + 1;
    fprintf(fptr, "(Team Manager)Hello, I am Team Manager For The Team Number: %d\n", ptr -> teamNumber);

    int start = -1;
    int end = -1;
    if(NUM_THREADS_PER_TEAM > subArraySize){
        printf("NUM_THREADS_PER_TEAM is too large For a randomly generated array size of %d, please re-run program\n", arraySize);
        pthread_exit(NULL);
    }
    
    
    int teamNumber = ptr -> teamNumber;
    for(int j = 0; j < NUM_THREADS_PER_TEAM; j ++){
        if(j == 0){
            start = ptr -> start;
            end = ptr -> start + (subArraySize / (NUM_THREADS_PER_TEAM)) - 1; 
        }
        else{
            start = end + 1;
            end = start + (subArraySize / (NUM_THREADS_PER_TEAM))  - 1; 
        }
        if(start > ptr -> end){
            start = ptr -> end;
        }
        if(end > ptr -> end){
            end = ptr -> end;
        }
        if(j == NUM_THREADS_PER_TEAM - 1){
            end = ptr -> end;
        }

        int ID = NUM_THREADS_PER_TEAM * teamNumber + j;

        teamMemberInformation = threadData[ID];
        teamMemberInformation -> end = end;
        teamMemberInformation -> start = start;
        teamMemberInformation -> teamNumber = teamNumber;
        teamMemberInformation -> threadNumber = j;
                
        pthread_attr_init(&threadAttributes[ID]);
        pthread_create(&threadIDs[ID], &threadAttributes[ID], threadFunction, threadData[ID]);
    }
    
    for(int j = 0; j < NUM_THREADS_PER_TEAM; j ++){
        int ID = NUM_THREADS_PER_TEAM * teamNumber + j;
        pthread_join(threadIDs[ID], NULL);
    }
    
    clock_t finishTime = clock() / CLOCKS_PER_SEC;
    double elapsed = ((double)(finishTime - begin)) / CLOCKS_PER_SEC;
    ptr -> elapsed = elapsed;
    ptr -> finishTime = finishTime;
    addTeamDataToQueue(ptr);
    pthread_exit(NULL);
    return NULL;
}

void generateThreadTeamManagers(){
    Subset * teamInformation = NULL;
    int start = -1;
    int end = -1;
    if(NUM_TEAMS > arraySize){
        printf("We cannot have the number of teams be greater than the size of the array. Please reconsider!\n");
        return;
    }
    for(int i = 0; i < NUM_TEAMS; i ++){
        start = end + 1;
        end =  (arraySize / (NUM_TEAMS)) * (i + 1); 
        if(start > arraySize - 1){
            start = arraySize - 1;
        }
        if(end > arraySize - 1){
            end = arraySize - 1;
        }
        if(i == NUM_TEAMS - 1){
            end = arraySize - 1;
        }
        teamInformation = teamData[i];
        teamInformation -> end = end;
        teamInformation -> start = start;
        teamInformation -> teamNumber = i;
        pthread_attr_init(&teamAttributes[i]);
        pthread_create(&teamIDs[i], &teamAttributes[i], teamFunction, teamData[i]);
    }

    for(int i = 0; i < NUM_TEAMS; i ++){
        pthread_join(teamIDs[i], NULL);
    }

    displayBulletinBoard();
    displayThreadFinishTimes();
    displayTeamFinishTimes();
}

void initialize(){
    threadIDs = (pthread_t *)(malloc(NUM_TEAMS * NUM_THREADS_PER_TEAM * sizeof(pthread_t)));
    threadAttributes = (pthread_attr_t *) (malloc(NUM_TEAMS * NUM_THREADS_PER_TEAM * sizeof(pthread_attr_t)));
    teamIDs = (pthread_t *)(malloc(NUM_TEAMS * sizeof(pthread_t)));
    teamAttributes = (pthread_attr_t *)(malloc(NUM_TEAMS * sizeof(pthread_attr_t)));
    teamData = (Subset**)(malloc(NUM_TEAMS * sizeof(Subset*)));
    threadData = (Subset **)(malloc(NUM_TEAMS * NUM_THREADS_PER_TEAM * sizeof(Subset *)));

    for(int i = 0; i < NUM_TEAMS; i ++){
        teamData[i] = (Subset *)(malloc(sizeof(Subset)));
    }

    for(int i = 0; i < NUM_TEAMS * NUM_THREADS_PER_TEAM; i ++){
        threadData[i] = (Subset *)(malloc(sizeof(Subset)));
    }

    HashTable = (int* )(malloc(50001 * sizeof(int)));
    threadFinishTimes = (Queue*)(malloc(sizeof(Queue)));
    teamFinishTimes = (Queue *)(malloc(sizeof(Queue)));
    sem_init(&queueMutexVariable, 0, 1);
    sem_init(&teamQueueMutexVariable, 0, 1);
}

void initializeBulletinBoard(){
    PrimeNumbersStorage = (BulletinBoard*)(malloc(sizeof(BulletinBoard)));
}


int main(){
    filename = "outputData.txt";
    fptr = fopen(filename, "w");
    printf("Please Enter a Number Indicating the Number of Thread Teams You Would Like to Generate\n");
    scanf("%d", &NUM_TEAMS);
    
    printf("Please Enter a Number Indicating the Number of Threads You Would Like to Have on Each Team\n");
    scanf("%d", &NUM_THREADS_PER_TEAM);
    generateRandomOddNumberArray();

    fprintf(fptr, "ARRAY SIZE: %d\n", arraySize);
    initialize();
    initializeBulletinBoard();

    generateThreadTeamManagers();
}