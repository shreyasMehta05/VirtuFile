#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>

time_t initTime;

#define _INVALID_OP_ -1
#define _READ_OP_ 0
#define _WRITE_OP_ 1
#define _DELETE_OP_ 2

#define _MAX_FILES_ 100
#define _MAX_USERS_ 100
#define _MAX_QUEUE_ 100

#define _SLEEP_TIME_ 100000

#define YELLOW "\033[1;33m"
#define PINK "\033[1;35m"
#define WHITE "\033[1;37m"
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define RESET "\033[0m"
#define BLUE "\033[1;34m"
#define CYAN "\033[1;36m"
#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"

typedef struct {
    bool isPresent;
    bool writeInProgress;
    bool deleteInProgress;
    int ReaderCount;
    int currentUserCount;
    int waiting_requests;
    pthread_mutex_t threadLock;
    pthread_cond_t signalCondition;
} FileStatus;

typedef struct {
    int userID;
    int fileID;
    int fileOperation;
    int reqTime;
} UserQuery;

int readTime, writeTime, deleteTime;
int fileCount, maxUsers, maxWaitTime;
UserQuery queryQueue[_MAX_QUEUE_];
UserQuery queryArray[_MAX_QUEUE_];
int queueFront = 0, queueRear = 0;
bool simulationComplete = false;
FileStatus fileStatus[_MAX_FILES_];
int queryCount = 0; // count of requests
pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueCondition = PTHREAD_COND_INITIALIZER;

void resetFileStatus();
void processInput();
void handleIncomingRequests(); 
int fetchCurrentTime();
void *processQuery(void *arg);
void pushQueryToQueue(UserQuery query);
UserQuery popQuery();
bool isQueueEmpty();
bool isQueueFull();
bool handleFileRead(UserQuery query, int fileID);
bool handleFileWrite(UserQuery query, int fileID);
bool handleFileDeletion(UserQuery query, int fileID);
bool initializeMain();
int compareQueries(const void *a, const void *b);

int main() {
    initializeMain();
    processInput();
    handleIncomingRequests();
    return 0;
}

bool initializeMain() {
    printf(GREEN "LAZY has woken up!\n" RESET);
    return true;
}

int fetchCurrentTime() {
    return (int)(time(NULL) - initTime);
}

void resetFileStatus() {
    for (int i = 0; i < _MAX_FILES_; i++) {
        fileStatus[i].isPresent = true;
        fileStatus[i].writeInProgress = false;
        fileStatus[i].deleteInProgress = false;
        fileStatus[i].ReaderCount = 0;
        fileStatus[i].currentUserCount = 0;
        fileStatus[i].waiting_requests = 0;
        pthread_mutex_init(&fileStatus[i].threadLock, NULL);
        pthread_cond_init(&fileStatus[i].signalCondition, NULL);
    }
}

int getUserId(char* inputBuffer) {
    int userID = atoi(inputBuffer);
    if (userID == 0) {
        printf(RED "Invalid User ID\n" RESET);
        return -1;
    }
    return userID;
}

void initialiseQuery(UserQuery* query, int userID, int fileID, int fileOperation, int reqTime) {
    query->userID = userID;
    query->fileID = fileID;
    query->fileOperation = fileOperation;
    query->reqTime = reqTime;
    return;
}

int mapOperationID(char* operation) {
    if (strcmp(operation, "READ") == 0) {
        return _READ_OP_;
    } else if (strcmp(operation, "WRITE") == 0) {
        return _WRITE_OP_;
    } else if (strcmp(operation, "DELETE") == 0) {
        return _DELETE_OP_;
    }
    return _INVALID_OP_;
}

// Comparison function for qsort
int compareQueries(const void *a, const void *b) {
    UserQuery *queryA = (UserQuery *)a;
    UserQuery *queryB = (UserQuery *)b;

    // First, sort by reqTime
    if (queryA->reqTime != queryB->reqTime) {
        return queryA->reqTime - queryB->reqTime;
    }

    // If times are the same, prioritize operations: READ < WRITE < DELETE
    if (queryA->fileOperation != queryB->fileOperation) {
        return queryA->fileOperation - queryB->fileOperation;
    }

    return 0;
}

void processInput() {
    scanf("%d %d %d", &readTime, &writeTime, &deleteTime);
    scanf("%d %d %d", &fileCount, &maxUsers, &maxWaitTime);

    char inputBuffer[20], command[100];
    int userID, fileID, reqTime;
    int queryIndex = 0;

    while (true) {
        scanf("%s", inputBuffer);
        if (strcmp(inputBuffer, "STOP") == 0) {
            break;
        }
        userID = getUserId(inputBuffer);
        scanf("%d %s %d", &fileID, command, &reqTime);
        int operation = mapOperationID(command);
        if (operation == _INVALID_OP_) {
            printf(RED "Invalid operation\n" RESET);
            continue;
        }
        initialiseQuery(&queryArray[queryIndex++], userID, fileID - 1, operation, reqTime);
    }

    // Sort the query array before pushing to queue
    qsort(queryArray, queryIndex, sizeof(UserQuery), compareQueries);

    // Push sorted queries to the queue
    for (int i = 0; i < queryIndex; i++) {
        pushQueryToQueue(queryArray[i]);
        queryCount++;
    }

    resetFileStatus();
    initTime = time(NULL);
}

void awaitRequestCompletion(pthread_t threads[]) {
    for (int i = 0; i < queryCount; i++) {
        pthread_join(threads[i], NULL);
    }
    simulationComplete = true;
    printf(GREEN "LAZY has no more pending requests and is going back to sleep!\n" RESET);
}

void handleIncomingRequests() {
    pthread_t threads[queryCount];
    for (int i = 0; i < queryCount; i++) {
        pthread_create(&threads[i], NULL, processQuery, NULL);
        usleep(1000);
    }

    awaitRequestCompletion(threads);
}

bool waitingPeriod(UserQuery query){
    while(fetchCurrentTime() < query.reqTime){
        usleep(_SLEEP_TIME_);
    }
    return true;
}
void mapOperation(char* operation, int fileOperation){
    if(fileOperation == _READ_OP_){
        strcpy(operation,"READ");
    }
    else if(fileOperation == _WRITE_OP_){
        strcpy(operation,"WRITE");
    }
    else if(fileOperation == _DELETE_OP_){
        strcpy(operation,"DELETE");
    }
    return;
}
bool executeOperation(UserQuery query){
    if(query.fileOperation == _READ_OP_){
        return handleFileRead(query,query.fileID);
    }
    else if(query.fileOperation == _WRITE_OP_){
        return handleFileWrite(query,query.fileID);
    }
    else if(query.fileOperation == _DELETE_OP_){
        return handleFileDeletion(query,query.fileID);
    }
    return false;
}

void *processQuery(void *arg){
    UserQuery query = popQuery();
    if(query.userID == -1){
        return NULL;
    }
    if(waitingPeriod(query)==false){
        return NULL;
    }
    char* operation = (char*)malloc(10*sizeof(char));
    mapOperation(operation,query.fileOperation);
    printf(YELLOW "User %d has made request for performing %s on file %d at %d seconds [YELLOW]\n" RESET,query.userID,operation,query.fileID+1,query.reqTime);
    free(operation);
    int processStartTime = query.reqTime + 1;
    bool success = false;
    success = executeOperation(query);
    return NULL;
}


bool waitForOneSecondHelper(UserQuery query,FileStatus* file){
    while(!(fetchCurrentTime() >= query.reqTime + 1)){
        pthread_mutex_unlock(&file->threadLock);
        usleep(_SLEEP_TIME_);
        pthread_mutex_lock(&file->threadLock);
    }
    return true;
}

bool waitForAccessRead(UserQuery query, FileStatus* file){
    struct timespec timeSpec;
    clock_gettime(CLOCK_REALTIME,&timeSpec);
    timeSpec.tv_sec += maxWaitTime-1;
    while(file->currentUserCount>=maxUsers){
        int timeout = pthread_cond_timedwait(&file->signalCondition,&file->threadLock,&timeSpec);
        if(timeout == ETIMEDOUT){
            break;
        }
    }
    return true;
}

bool checkResponseTime(UserQuery query){
    // printf("Max wait time: %d\n",maxWaitTime);
    int currentTime = fetchCurrentTime();
    if(currentTime - query.reqTime >= maxWaitTime){
        printf(RED "User %d canceled the request due to no response at %d seconds [RED]\n" RESET,query.userID,query.reqTime + maxWaitTime);
        return false;
    }
    return true;
}
bool checkCondForRead(UserQuery query, FileStatus* file){
    // check if file is present and not being deleted
    if(query.fileID >= fileCount || !fileStatus[query.fileID].isPresent || fileStatus[query.fileID].deleteInProgress){
        printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested. [WHITE]\n" RESET,query.userID,fetchCurrentTime());
        return false;
    }
    return true;
}

void startRead(UserQuery query, FileStatus* file){
    printf(PINK "LAZY has taken up the request of User %d to READ at %d seconds [PINK]\n" RESET,query.userID,fetchCurrentTime());
    file->currentUserCount++;
    file->ReaderCount++;
    return;
}
void finishRead(UserQuery query, FileStatus* file){
    printf(GREEN "The request for User %d was completed at %d seconds [GREEN]\n" RESET,query.userID,fetchCurrentTime());
    file->currentUserCount--;
    file->ReaderCount--;
    return;
}

bool handleFileRead(UserQuery query, int fileID){
    FileStatus* file = &fileStatus[fileID];
    pthread_mutex_lock(&file->threadLock);
    file->waiting_requests++;

    if(!waitForOneSecondHelper(query,file) || !waitForAccessRead(query,file) || !checkResponseTime(query) || !checkCondForRead(query,file)){
        file->waiting_requests--;
        pthread_mutex_unlock(&file->threadLock);
        return false;
    }
    file->waiting_requests--;

    // Start the read operation
    startRead(query,file);
    pthread_mutex_unlock(&file->threadLock);
    
    // simulate reading
    sleep(readTime);

    // complete the read operation
    pthread_mutex_lock(&file->threadLock);
    finishRead(query,file);
    for (int i=0;i<file->waiting_requests;i++)
    {
        pthread_cond_signal(&file->signalCondition);
        usleep(1000);
    }
    pthread_mutex_unlock(&file->threadLock);
    return true;
}

bool checkCondForWrite(UserQuery query, FileStatus* file){
    // check if file is present and not being deleted
    if(query.fileID >= fileCount || !fileStatus[query.fileID].isPresent || fileStatus[query.fileID].deleteInProgress){
        printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested. [WHITE]\n" RESET,query.userID,fetchCurrentTime());
        return false;
    }
    return true;
}
bool waitForAccessWrite(UserQuery query, FileStatus* file){
    struct timespec timeSpec;
    clock_gettime(CLOCK_REALTIME,&timeSpec);
    timeSpec.tv_sec += maxWaitTime-1;
    while(file->currentUserCount>=maxUsers||file->writeInProgress){
        int timeout = pthread_cond_timedwait(&file->signalCondition,&file->threadLock,&timeSpec);
        if(timeout == ETIMEDOUT){
            break;
        }
    }
    return true;
}


void startWrite(UserQuery query, FileStatus* file){
    printf(PINK "LAZY has taken up the request of User %d to WRITE at %d seconds [PINK]\n" RESET,query.userID,fetchCurrentTime());
    file->currentUserCount++;
    file->writeInProgress = true;
    return;
}
void completeWrite(UserQuery query, FileStatus* file){
    printf(GREEN "The request for User %d was completed at %d seconds [GREEN]\n" RESET,query.userID,fetchCurrentTime());
    file->currentUserCount--;
    file->writeInProgress = false;
    return;
}

bool handleFileWrite(UserQuery query, int fileID){
    FileStatus* file = &fileStatus[fileID];
    pthread_mutex_lock(&file->threadLock);

    file->waiting_requests++;
    if(!waitForOneSecondHelper(query,file) || !waitForAccessWrite(query,file) || !checkResponseTime(query) || !checkCondForWrite(query,file)){
        file->waiting_requests--;
        pthread_mutex_unlock(&file->threadLock);
        return false;
    }
    file->waiting_requests--;

    // Start the write operation
    startWrite(query,file);
    pthread_mutex_unlock(&file->threadLock);

    // simulate writing
    sleep(writeTime);

    // complete the write operation
    pthread_mutex_lock(&file->threadLock);
    completeWrite(query,file);
    for (int i=0;i<file->waiting_requests;i++)
    {
        pthread_cond_signal(&file->signalCondition);
        usleep(1000);
    }
    pthread_mutex_unlock(&file->threadLock);

    return true;  
}

bool checkCondForDelete(UserQuery query, FileStatus* file){
    // check if file is present and not being deleted
    if(query.fileID >= fileCount || !fileStatus[query.fileID].isPresent || fileStatus[query.fileID].deleteInProgress){
        printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested. [WHITE]\n" RESET,query.userID,fetchCurrentTime());
        return false;
    }
    return true;
}

void startDelete(UserQuery query, FileStatus* file){
    printf(PINK "LAZY has taken up the request of User %d to DELETE at %d seconds [PINK]\n" RESET,query.userID,fetchCurrentTime());
    file->deleteInProgress = true;
    file->currentUserCount++;
    return;
}
void completeDelete(UserQuery query, FileStatus* file){
    printf(GREEN "The request for User %d was completed at %d seconds [GREEN]\n" RESET,query.userID,fetchCurrentTime());
    file->currentUserCount--;
    file->deleteInProgress = false;
    file->isPresent = false;
    return;
}
bool waitForAccessDelete(UserQuery query, FileStatus* file){
    struct timespec timeSpec;
    clock_gettime(CLOCK_REALTIME,&timeSpec);
    timeSpec.tv_sec += maxWaitTime-1;
    while(file->currentUserCount>=maxUsers||file->writeInProgress || file->ReaderCount>0){
        int timeout = pthread_cond_timedwait(&file->signalCondition,&file->threadLock,&timeSpec);
        if(timeout == ETIMEDOUT){
            break;
        }
    }
    return true;
}

bool handleFileDeletion(UserQuery query, int fileID){
    FileStatus* file = &fileStatus[fileID];
    pthread_mutex_lock(&file->threadLock);
    file->waiting_requests++;

    if(!waitForOneSecondHelper(query,file) || !waitForAccessDelete(query,file) || !checkResponseTime(query) || !checkCondForDelete(query,file)){
        file->waiting_requests--;
        pthread_mutex_unlock(&file->threadLock);
        return false;
    }
    file->waiting_requests--;
    // Start the delete operation
    startDelete(query,file);
    pthread_mutex_unlock(&file->threadLock);

    // simulate delete operation
    sleep(deleteTime);

    // complete the delete operation
    pthread_mutex_lock(&file->threadLock);
    completeDelete(query,file);
    pthread_cond_broadcast(&file->signalCondition);
    pthread_mutex_unlock(&file->threadLock);
    return true;
}

bool isQueueEmpty(){ 
    // already locked
    return queueFront == queueRear;
}
bool isQueueFull(){
    // already locked
    return (queueRear+1)%_MAX_QUEUE_ == queueFront;
}
void pushQueryToQueue(UserQuery query){
    pthread_mutex_lock(&queueLock);
    if(isQueueFull()){
        printf(RED "Queue is full\n" RESET);
        pthread_mutex_unlock(&queueLock);
        return;
    }
    queryQueue[queueRear] = query;
    queueRear = (queueRear+1)%_MAX_QUEUE_;
    pthread_cond_signal(&queueCondition);
    pthread_mutex_unlock(&queueLock);
}

UserQuery popQuery(){
    pthread_mutex_lock(&queueLock);
    while(isQueueEmpty() && !simulationComplete){
        pthread_cond_wait(&queueCondition,&queueLock);
    }
    UserQuery query;
    initialiseQuery(&query,_INVALID_OP_,_INVALID_OP_,_INVALID_OP_,_INVALID_OP_);
    if(!simulationComplete && !isQueueEmpty()){
        query = queryQueue[queueFront];
        queueFront = (queueFront+1)%_MAX_QUEUE_;
    }
    pthread_mutex_unlock(&queueLock);
    return query;
}