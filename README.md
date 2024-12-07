# **Lazy File Management System (LFMS)** 🚀  

## **Author**  
👤 **Shreyas Mehta**  

---

## **Overview**  

The **Lazy File Management System (LFMS)** simulates a **file management system** where multiple users access files **concurrently**. It supports operations like **read**, **write**, and **delete** while ensuring **synchronization** and **mutual exclusion**.  

🔑 **Key Features**:  
- Uses **pthreads** to handle **concurrent requests**.  
- Ensures **thread safety** with **mutexes** and **condition variables**.  
- Guarantees proper synchronization for all operations.  

This project was implemented in **C** as part of the course *Operating Systems and Networks*.  

---

## **Key Data Structures**  

### **1. `UserQuery` Structure** 📝  
A `UserQuery` represents a single user request and includes:  
- **`userID`**: Identifier of the user.  
- **`fileID`**: Identifier of the file.  
- **`fileOperation`**: Operation to perform (📖 `0 = Read`, ✏️ `1 = Write`, ❌ `2 = Delete`).  
- **`reqTime`**: Request timestamp in seconds.  

---

### **2. `FileStatus` Structure** 📂  
A `FileStatus` tracks the state of a file:  
- **`isPresent`**: Indicates if the file exists.  
- **`writeInProgress`**: Flags if a write operation is ongoing.  
- **`deleteInProgress`**: Flags if the file is being deleted.  
- **`ReaderCount`**: Number of active readers.  
- **`currentUserCount`**: Total users accessing the file.  
- **`threadLock`**: Mutex for synchronizing file access.  
- **`signalCondition`**: Condition variable to manage threads waiting for access.  


## **Highlights**  
✨ Implements **thread-safe file operations** using **pthreads**.  
✨ Synchronizes **concurrent user requests** with **mutexes** and **condition variables**.  
✨ Efficient handling of **read**, **write**, and **delete** operations.  

## **Functions Breakdown**

### **1. `resetFileStatus()`**

#### **Description:**
This function resets the status of all files in the system to their initial state, ensuring that no operations (read, write, or delete) are ongoing and that files are available for future operations.

```c
void resetFileStatus() {
    for (int i = 0; i < MAX_FILES; i++) {
        fileStatus[i].isPresent = true;
        fileStatus[i].writeInProgress = false;
        fileStatus[i].deleteInProgress = false;
        fileStatus[i].ReaderCount = 0;
        fileStatus[i].currentUserCount = 0;
    }
}
```

---

### **2. `processInput()`**

#### **Description:**
This function processes user input, which could include commands to read, write, or delete files. The input is parsed and translated into a `UserQuery` that is pushed to the query queue for further processing.

```c
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
```

---

### **3. `handleIncomingRequests()`**

#### **Description:**
This function handles incoming requests from the query queue, processes them, and delegates the appropriate actions (read, write, or delete) to the relevant functions like `handleFileRead()`, `handleFileWrite()`, and `handleFileDeletion()`.

```c
void handleIncomingRequests() {
    while (!isQueueEmpty()) {
        UserQuery query = popQuery();
        int fileID = query.fileID;

        switch (query.fileOperation) {
            case 0:
                handleFileRead(query, fileID);
                break;
            case 1:
                handleFileWrite(query, fileID);
                break;
            case 2:
                handleFileDeletion(query, fileID);
                break;
            default:
                // Invalid operation
                break;
        }
    }
}
```

---

### **4. `fetchCurrentTime()`**

#### **Description:**
This function retrieves the current system time in seconds, typically used for timestamping queries or tracking the elapsed time.

```c
int fetchCurrentTime() {
    return (int)time(NULL); // Return current time in seconds
}
```

---

### **5. `processQuery(void *arg)`**

#### **Description:**
This function is the entry point for each worker thread. It processes a single query from the queue and invokes the relevant file operation (read, write, or delete).

```c
void *processQuery(void *arg) {
    UserQuery *query = (UserQuery *)arg;
    int fileID = query->fileID;

    // Handle the query based on its type
    switch (query->fileOperation) {
        case 0:
            handleFileRead(*query, fileID);
            break;
        case 1:
            handleFileWrite(*query, fileID);
            break;
        case 2:
            handleFileDeletion(*query, fileID);
            break;
        default:
            break;
    }

    return NULL;
}
```

---

### **6. `pushQueryToQueue(UserQuery query)`**

#### **Description:**
This function pushes a `UserQuery` onto the query queue. If the queue is full, it waits for space to become available before pushing the query.

```c
void pushQueryToQueue(UserQuery query) {
    // Lock the queue before modifying it
    pthread_mutex_lock(&queueLock);

    while (isQueueFull()) {
        pthread_cond_wait(&queueNotFull, &queueLock);
    }

    // Add the query to the queue
    queryQueue[rear] = query;
    rear = (rear + 1) % QUEUE_SIZE;

    // Signal that the queue is not empty
    pthread_cond_signal(&queueNotEmpty);
    pthread_mutex_unlock(&queueLock);
}
```

---

### **7. `popQuery()`**

#### **Description:**
This function pops a `UserQuery` from the front of the query queue. If the queue is empty, it waits until a query becomes available.

```c
UserQuery popQuery() {
    pthread_mutex_lock(&queueLock);

    while (isQueueEmpty()) {
        pthread_cond_wait(&queueNotEmpty, &queueLock);
    }

    UserQuery query = queryQueue[front];
    front = (front + 1) % QUEUE_SIZE;

    pthread_cond_signal(&queueNotFull);
    pthread_mutex_unlock(&queueLock);

    return query;
}
```

---

### **8. `isQueueEmpty()`**

#### **Description:**
This function checks whether the query queue is empty.

```c
bool isQueueEmpty() {
    return (front == rear);
}
```

---

### **9. `isQueueFull()`**

#### **Description:**
This function checks whether the query queue is full.

```c
bool isQueueFull() {
    return ((rear + 1) % QUEUE_SIZE == front);
}
```

---

### **10. `handleFileRead(UserQuery query, int fileID)`**

#### **Description:**
This function handles the read operation for a file specified by `fileID`. It ensures that the file is available for reading, performs necessary checks, and simulates the read operation.

```c
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
```

---

### **11. `handleFileWrite(UserQuery query, int fileID)`**

#### **Description:**
This function handles the write operation for a specific file identified by `fileID`. It ensures that the file is available for writing and performs necessary checks.

```c
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
```

---

### **12. `handleFileDeletion(UserQuery query, int fileID)`**

#### **Description:**
This function handles the delete operation for a specific file identified by `fileID`. It ensures that the file is available for deletion.

```c
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
```

---

### **13. `initializeMain()`**

#### **Description:**
This function initializes the main system, including setting up mutexes, condition variables, and the query queue.

```c
bool initializeMain() {
    // Initialize mutexes and condition variables
    pthread_mutex_init(&queueLock, NULL);
    pthread_cond_init(&queueNotEmpty, NULL);
    pthread_cond_init(&queueNotFull, NULL);
    return true;
}
```

---

### **14. `compareQueries(const void *a, const void *b)`**

#### **Description:**
This function compares two `UserQuery` structures to sort them based on their request time.

```c
int compareQueries(const void *a, const void *b) {
    return ((UserQuery*)a)->reqTime - ((UserQuery*)b)->reqTime;
}
```

---


#### 15. **checkCondForDelete**
```c
bool checkCondForDelete(UserQuery query, FileStatus* file) {
    // Check if file exists and is not being deleted.
    pthread_mutex_lock(&file->fileLock);
    if (!file->isPresent || file->deleteInProgress) {
        pthread_mutex_unlock(&file->fileLock);
        return false;
    }
    pthread_mutex_unlock(&file->fileLock);
    return true;
}
```
**Description:**  
This function checks if the file is present and not in the process of being deleted before allowing the delete operation to proceed. It uses a mutex lock to ensure thread safety.

---

#### 16. **startDelete**
```c
void startDelete(UserQuery query, FileStatus* file) {
    // Start the delete operation for a file.
    pthread_mutex_lock(&file->fileLock);
    file->deleteInProgress = true;
    file->userCount++;
    pthread_mutex_unlock(&file->fileLock);
}
```
**Description:**  
This function marks a file as being in the process of deletion and increments the user count to track the number of operations on the file.

---

#### 17. **completeDelete**
```c
void completeDelete(UserQuery query, FileStatus* file) {
    // Complete the delete operation for the file.
    pthread_mutex_lock(&file->fileLock);
    file->deleteInProgress = false;
    file->userCount--;
    file->isPresent = false; // File is deleted and no longer available
    pthread_mutex_unlock(&file->fileLock);
}
```
**Description:**  
Marks the delete operation as complete, decrements the user count, and sets the file's `isPresent` flag to `false`, indicating the file is no longer available.

---

#### 18. **waitForAccessDelete**
```c
bool waitForAccessDelete(UserQuery query, FileStatus* file) {
    // Wait until the file is available for deletion (not being accessed by other users).
    pthread_mutex_lock(&file->fileLock);
    if (file->deleteInProgress || file->userCount > 0) {
        pthread_mutex_unlock(&file->fileLock);
        return false; // Cannot access the file if it's being deleted or in use
    }
    pthread_mutex_unlock(&file->fileLock);
    return true;
}
```
**Description:**  
This function ensures the file is not being deleted or accessed by other users before allowing a delete operation to proceed. It uses a mutex lock to prevent race conditions.

---

#### 19. **checkCondForWrite**
```c
bool checkCondForWrite(UserQuery query, FileStatus* file) {
    // Check if the file exists and is not being deleted or written to.
    pthread_mutex_lock(&file->fileLock);
    if (!file->isPresent || file->writeInProgress || file->deleteInProgress) {
        pthread_mutex_unlock(&file->fileLock);
        return false;
    }
    pthread_mutex_unlock(&file->fileLock);
    return true;
}
```
**Description:**  
Checks the conditions necessary for performing a write operation: the file must exist, and it must not be under deletion or have an ongoing write operation.

---

#### 20. **waitForAccessWrite**
```c
bool waitForAccessWrite(UserQuery query, FileStatus* file) {
    // Wait until the file is available for writing (not being written to).
    pthread_mutex_lock(&file->fileLock);
    if (file->writeInProgress || file->userCount >= 1) {
        pthread_mutex_unlock(&file->fileLock);
        return false; // Cannot access file if it's being written to or being used by other users
    }
    pthread_mutex_unlock(&file->fileLock);
    return true;
}
```
**Description:**  
Waits for the file to be available for writing. If the file is being written to or is being accessed by other users, the function will return `false` and wait until the conditions are met.

---

#### 21. **startWrite**
```c
void startWrite(UserQuery query, FileStatus* file) {
    // Start the write operation for a file.
    pthread_mutex_lock(&file->fileLock);
    file->writeInProgress = true;
    file->userCount++;
    pthread_mutex_unlock(&file->fileLock);
}
```
**Description:**  
This function starts the write operation for a file, marking it as "in-progress" and incrementing the user count to track the number of operations on the file.

---

#### 22. **completeWrite**
```c
void completeWrite(UserQuery query, FileStatus* file) {
    // Complete the write operation for the file.
    pthread_mutex_lock(&file->fileLock);
    file->writeInProgress = false;
    file->userCount--;
    pthread_mutex_unlock(&file->fileLock);
}
```
**Description:**  
Completes the write operation, marks the file as no longer being written to, and decrements the user count.

---

#### 23. **waitForOneSecondHelper**
```c
bool waitForOneSecondHelper(UserQuery query, FileStatus* file) {
    // Ensure the file operation is delayed by at least one second.
    while (!(fetchCurrentTime() >= query.reqTime + 1)) {
        pthread_mutex_unlock(&file->fileLock);
        usleep(__SLEEP_TIME__);
        pthread_mutex_lock(&file->fileLock);
    }
    return true;
}
```
**Description:**  
Waits for at least one second after the request time before allowing the file operation to proceed. This ensures that file operations are not executed immediately, allowing the system to manage concurrency.

---

#### 24. **waitForAccessRead**
```c
bool waitForAccessRead(UserQuery query, FileStatus* file) {
    // Wait until the file is available for reading.
    pthread_mutex_lock(&file->fileLock);
    if (file->userCount >= maxUsers || file->writeInProgress) {
        pthread_mutex_unlock(&file->fileLock);
        return false; // File cannot be read if it has too many users or is being written to
    }
    pthread_mutex_unlock(&file->fileLock);
    return true;
}
```
**Description:**  
Waits for the file to be available for reading. If the file is being written to or has too many users accessing it, the function will return `false` and wait until conditions are met.

---

#### 25. **checkResponseTime**
```c
bool checkResponseTime(UserQuery query) {
    // Check if the response time has exceeded the maximum allowed wait time.
    int currentTime = fetchCurrentTime();
    if (currentTime - query.reqTime >= maxWaitTime) {
        printf(RED "User %d canceled the request due to no response at %d seconds [RED]\n" RESET, query.userID, query.reqTime + maxWaitTime);
        return false;
    }
    return true;
}
```
**Description:**  
This function checks if the response time for a request exceeds the maximum allowed wait time. If it does, it cancels the request and returns `false`.

---

#### 26. **checkCondForRead**
```c
bool checkCondForRead(UserQuery query, FileStatus* file) {
    // Check if the file exists and is not being deleted.
    pthread_mutex_lock(&file->fileLock);
    if (!file->isPresent || file->deleteInProgress) {
        pthread_mutex_unlock(&file->fileLock);
        return false;
    }
    pthread_mutex_unlock(&file->fileLock);
    return true;
}
```
**Description:**  
Ensures that the file exists and is not being deleted before allowing a read operation to proceed.

---

#### 27. **startRead**
```c
void startRead(UserQuery query, FileStatus* file) {
    // Start the read operation for a file.
    pthread_mutex_lock(&file->fileLock);
    file->userCount++;
    file->ReaderCount++;
    pthread_mutex_unlock(&file->fileLock);
}
```
**Description:**  
Marks the file as being accessed for reading and increments the count of active readers.

---

#### 28. **finishRead**
```c
void finishRead(UserQuery query, FileStatus* file) {
    // Complete the read operation for the file.
    pthread_mutex_lock(&file->fileLock);
    file->userCount--;
    file->ReaderCount--;
    pthread_mutex_unlock(&file->fileLock);
}
```
**Description:**  
Marks the read operation as completed, decrementing the user count and the reader count.

---

#### 29. **awaitRequestCompletion**
```c
void awaitRequestCompletion(pthread_t threads[]) {
    // Wait for all threads to complete before proceeding.
    for (int i = 0; i < queryCount; i++) {
        pthread_join(threads[i], NULL);
    }
    simulationComplete = true;
    printf(GREEN "LAZY has no more pending requests and is going back to sleep!\n" RESET);
}
```
**Description:**  
Waits for all threads (file operation requests) to complete before ending the simulation and going back to sleep.

---

#### 30. **handleIncomingRequests**
```c
void handleIncomingRequests() {
    pthread_t threads[queryCount];
    for (int i = 0; i < queryCount; i++) {
        pthread_create(&threads[i], NULL,

 executeOperation, (void*)&userRequests[i]);
    }
    awaitRequestCompletion(threads);
}
```
**Description:**  
Handles incoming requests by spawning a new thread for each user request and awaiting their completion.

---

#### 31. **waitingPeriod**
```c
bool waitingPeriod(UserQuery query) {
    int currentTime = fetchCurrentTime();
    return currentTime - query.reqTime < maxWaitTime;
}
```
**Description:**  
Determines if the wait time has expired for a given user request, ensuring that no operation is allowed beyond the defined maximum wait time.

---

### **Mapping File Operations**

```c
void mapOperation(char* operation, int fileOperation) {
    // Map the user request to the corresponding file operation
    switch (fileOperation) {
        case 0: // Read
            executeReadOperation();
            break;
        case 1: // Write
            executeWriteOperation();
            break;
        case 2: // Delete
            executeDeleteOperation();
            break;
        default:
            printf("Invalid operation\n");
    }
}
```

**Description:**  
Maps the requested file operation to the corresponding function, ensuring that the correct file operation is executed based on the user's input.

---

### **Conclusion**

The system ensures that file operations (read, write, and delete) are performed safely and concurrently using mutex locks to prevent race conditions. Each function is responsible for handling specific parts of the file operation, ensuring proper synchronization and avoiding conflicts between different user requests.