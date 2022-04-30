//Action 2

#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct Node{
    int value;
    struct Node * nextNode;
    int hasNext;
    int head;

    int highestPrimeNumber; //For numbers that are not prime numbers, this field will indicate the highest prime number that is less than the given number
    int numPrimeNumbers;    //For numbers that are not prime numbers, this field will indicate the number of prime numbers that are before this number
} Node;

int * ARRAY;
int arraySize;

int NUM_TEAMS;
int NUM_THREADS_PER_TEAM;

pthread_t* threadIDs;
pthread_attr_t* threadAttributes;

pthread_t* teamIDs;
pthread_attr_t* teamAttributes;

pthread_mutex_t mutexVariable = PTHREAD_MUTEX_INITIALIZER;
typedef struct Subset{
    int start;
    int end;
    int teamNumber;
} Subset;

Subset ** teamData;

Subset ** threadData;

typedef struct BulletinBoard{
    int primeNumbersIdentified;

    Node * primeNumbersFound;

    Node * nonPrimeNumbersFound;
} BulletinBoard;


BulletinBoard * PrimeNumbersStorage;

void addPrimeToBulletinBoard(int num){
    Node * listFront = PrimeNumbersStorage -> primeNumbersFound;
    Node * ptr = listFront;
    if(ptr -> hasNext == 0){
        Node * newNode = (Node *)(malloc(sizeof(Node)));
        newNode -> hasNext = 0;
        newNode -> head = 0;
        newNode -> value = num;

        ptr -> nextNode = newNode;
        ptr -> hasNext = 1;

        PrimeNumbersStorage -> primeNumbersIdentified ++;
        return;
    }

    while(ptr -> hasNext == 1){
        Node * adjacentNode = ptr -> nextNode;

        if(num >= ptr -> value && num <= adjacentNode -> value){
            Node * newNode = (Node *)(malloc(sizeof(Node)));
            newNode -> value = num;
            newNode -> hasNext = 1;
            newNode -> head = 0;

            newNode -> nextNode = adjacentNode;

            ptr -> nextNode = newNode;
            ptr -> hasNext = 1;

            PrimeNumbersStorage -> primeNumbersIdentified ++;
            return;
        }
    }

    Node * newNode = (Node *)(malloc(sizeof(Node)));
    newNode -> value = num;
    newNode -> hasNext = 0;
    newNode -> head = 0;

    ptr -> nextNode = newNode;
    ptr -> hasNext = 1;

    PrimeNumbersStorage -> primeNumbersIdentified ++;
}

void addNonPrimeNumberToBulletinBoard(int num, int numProcessed, int highestPrime){
    Node * listFront = PrimeNumbersStorage -> nonPrimeNumbersFound;
    Node * ptr = listFront;
    if(ptr -> hasNext == 0){
        Node * newNode = (Node *)(malloc(sizeof(Node)));
        newNode -> hasNext = 0;
        newNode -> head = 0;
        newNode -> value = num;
        newNode -> highestPrimeNumber = highestPrime;
        newNode -> numPrimeNumbers = numProcessed;

        ptr -> nextNode = newNode;
        ptr -> hasNext = 1;

        return;
    }

    while(ptr -> hasNext == 1){
        Node * adjacentNode = ptr -> nextNode;

        if(num >= ptr -> value && num <= adjacentNode -> value){
            Node * newNode = (Node *)(malloc(sizeof(Node)));
            newNode -> value = num;
            newNode -> hasNext = 1;
            newNode -> head = 0;
            newNode -> nextNode = adjacentNode;
            newNode -> highestPrimeNumber = highestPrime;
            newNode -> numPrimeNumbers = numProcessed;

            ptr -> nextNode = newNode;
            ptr -> hasNext = 1;

            return;
        }
    }

    Node * newNode = (Node *)(malloc(sizeof(Node)));
    newNode -> value = num;
    newNode -> hasNext = 0;
    newNode -> head = 0;
    newNode -> highestPrimeNumber = highestPrime;
    newNode -> numPrimeNumbers = numProcessed;

    ptr -> nextNode = newNode;
    ptr -> hasNext = 1;

}

int isPrimeNumberInBulletinBoard(int num){
    int alreadyPresent = 0;
    Node * listFront = PrimeNumbersStorage -> primeNumbersFound;
    Node * ptr = listFront;
    if(ptr -> hasNext == 0){
        return 0;
    }

    while(ptr -> hasNext == 1){
        ptr = ptr -> nextNode;
        if(ptr -> value == num){
            alreadyPresent = 1;
            break;
        }
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
    int counter = 1;

    int prime = 0;

    for(int counter = 1; counter < number; counter ++){
        if(number % counter == 0){
            prime = 1;
            break;
        }
    }

    return prime;
}

int generateRandomOddNumberArray(){
    srand ( time(NULL) );
    
    arraySize = (rand() % 10) + 100;

    ARRAY = malloc(arraySize * sizeof(int));

    for(int i = 0; i < arraySize; i ++){
        ARRAY[i] = (rand() % 10) + 5;
    }

    return 0;
}

void * threadFunction(void * arg){
    Subset * ptr = (Subset*)(arg);
    printf("(Team Member) Hi My name is Thread %d and I am a member of team %d, Start: %d, End: %d\n", (int)(pthread_self()), ptr -> teamNumber, ptr -> start, ptr -> end);

    int start = ptr -> start;
    int end = ptr -> end;

    int primesProcessed = 0;
    int highestPrimeProcessed = -1;
    for(int counter = start; counter <= end; counter ++){
        pthread_mutex_lock(&mutexVariable);
        int numToAnalyze = *(ARRAY + counter);
        int alreadyPresent = isPrimeNumberInBulletinBoard(numToAnalyze);
        if(alreadyPresent == 1){
            pthread_mutex_unlock(&mutexVariable);
            continue;
        }

        int prime = isPrime(numToAnalyze);
        if(prime == 1){
            addPrimeToBulletinBoard(numToAnalyze);
            primesProcessed ++;
            highestPrimeProcessed = maximum(highestPrimeProcessed, numToAnalyze);
            pthread_mutex_unlock(&mutexVariable);
            continue;
        }
        else{
            addNonPrimeNumberToBulletinBoard(numToAnalyze, primesProcessed, highestPrimeProcessed);
        }

        pthread_mutex_unlock(&mutexVariable);
    }
    return NULL;
}

void * teamFunction(void * arg){
    Subset * ptr = (Subset*)(arg);
    int subArraySize = ptr -> end - ptr -> start + 1;
    printf("(Team Manager)Team Number: %d, Start: %d, End: %d\n", ptr -> teamNumber, ptr -> start, ptr -> end);
    

    Subset * teamData = (Subset *)(malloc(sizeof(Subset)));

    if(NUM_THREADS_PER_TEAM > ptr -> end - ptr -> start + 1){
        printf("NUM_THREADS_PER_TEAM is too large\n");
        pthread_exit(NULL);
    }

    int i = ptr -> teamNumber;

    for(int j = 0; j < NUM_THREADS_PER_TEAM; j ++){
        if(j == 0){
            teamData -> start = ptr -> start;
            teamData -> end = ptr -> start + (subArraySize / (NUM_THREADS_PER_TEAM)) - 1; 
        }
        else{
            teamData -> start = teamData -> end + 1;
            teamData -> end = teamData -> start + (subArraySize / (NUM_THREADS_PER_TEAM))  - 1; 
        }

        if(j == NUM_THREADS_PER_TEAM - 1){
            teamData -> end = ptr -> end;
        }

        Subset * item = threadData[NUM_TEAMS * i + j];
        item -> end = teamData -> end;
        item -> start = teamData -> start;
        item -> teamNumber = i;

        pthread_attr_init(&threadAttributes[NUM_TEAMS * i + j]);
        pthread_create(&threadIDs[NUM_TEAMS * i + j], &threadAttributes[NUM_TEAMS * i + j], threadFunction, item);
    }

    for(int j = 0; j < NUM_THREADS_PER_TEAM; j ++){
        pthread_join(threadIDs[NUM_TEAMS * i + j], NULL);
    }

    return NULL;
}

int main(){
    generateRandomOddNumberArray(); 
    
    NUM_TEAMS = 5;
    NUM_THREADS_PER_TEAM = 2;

    PrimeNumbersStorage = (BulletinBoard*)(malloc(sizeof(BulletinBoard)));
    PrimeNumbersStorage->primeNumbersFound = (Node*)(malloc(arraySize * sizeof(Node)));

    PrimeNumbersStorage -> primeNumbersFound -> hasNext = 0;
    PrimeNumbersStorage -> primeNumbersFound -> value = -1;
    PrimeNumbersStorage -> primeNumbersFound -> head = 1;
    PrimeNumbersStorage -> primeNumbersFound -> nextNode = (Node*)(malloc(arraySize * sizeof(Node)));

    PrimeNumbersStorage->nonPrimeNumbersFound = (Node*)(malloc(arraySize * sizeof(Node)));

    PrimeNumbersStorage -> nonPrimeNumbersFound -> hasNext = 0;
    PrimeNumbersStorage -> nonPrimeNumbersFound -> value = -1;
    PrimeNumbersStorage -> nonPrimeNumbersFound -> head = 1;
    PrimeNumbersStorage -> nonPrimeNumbersFound -> nextNode = (Node*)(malloc(arraySize * sizeof(Node)));

    

    

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

    Subset current = {-1, -1};
    
    
    
    if(NUM_TEAMS > arraySize){
        printf("We cannot have the number of teams be greater than the size of the array. Please reconsider!\n");
        return 1;
    }
    for(int i = 0; i < NUM_TEAMS; i ++){
        current.start = current.end + 1;
        current.end =  (arraySize / (NUM_TEAMS)) * (i + 1); 
        if(i == NUM_TEAMS - 1){
            current.end = arraySize - 1;
        }
        Subset * ptr = teamData[i];
        ptr -> end = current.end;
        ptr -> start = current.start;
        ptr -> teamNumber = i;
        pthread_attr_init(&teamAttributes[i]);
        pthread_create(&teamIDs[i], &teamAttributes[i], teamFunction, teamData[i]);
    }
    
    /*
    printf("(Main Thread) Waiting for Threads to Complete\n");
    for(int i = 0; i < NUM_TEAMS; i ++){
        pthread_join(teamIDs[i], NULL);
    }
    */


}