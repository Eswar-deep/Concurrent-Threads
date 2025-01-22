#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

#define MAX_STUDENTS 10000
 
// Struct student
// id is thread id
// Priority is no:of helps recieved, high the priority value low the priority
typedef struct 
{
    int id;
    int priority;
    int arrival_time;
} Student;

// Shared sata structures
// Between student and coordinator
Student coordinatorQueue[MAX_STUDENTS];
// Between tutor and coordinator
Student tutorQueue[MAX_STUDENTS];
// Count of students in each queue
int coordinatorQueueCount = 0;
int tutorQueueCount = 0;

// Coordinator queue enqueue (priority queue)
// Priority stores the helps recieved, low priority in low index
// Same priority we check arrival time
void enqueueCoordinatorQueue(Student s) 
{
    int i = coordinatorQueueCount - 1;
    while (i >= 0) 
    {
        if(coordinatorQueue[i].priority > s.priority || (coordinatorQueue[i].priority == s.priority && coordinatorQueue[i].arrival_time < s.arrival_time)) 
        {
            coordinatorQueue[i + 1] = coordinatorQueue[i];
        }
        else
        {
            break;
        }
        i--;
    }
    coordinatorQueue[i + 1] = s;
    coordinatorQueueCount++;
}

// Coordinator queue dequeue (priority queue)
Student dequeueCoordinatorQueue() 
{
    Student s = coordinatorQueue[0];
    for (int i = 1; i < coordinatorQueueCount; i++) 
    {
        coordinatorQueue[i - 1] = coordinatorQueue[i];
    }
    coordinatorQueueCount--;
    return s;
}

// Tutor queue enqueue (FIFO queue)
void enqueueTutorQueue(Student s) 
{
    tutorQueue[tutorQueueCount++] = s;
}

// Tutor queue dequeue (FIFO queue)
Student dequeueTutorQueue() 
{
    Student s = tutorQueue[0];
    for (int i = 1; i < tutorQueueCount; i++) 
    {
        tutorQueue[i - 1] = tutorQueue[i];
    }
    tutorQueueCount--;
    return s;
}

// Arrival order tracking
int getCurrentTime() 
{
    static int time = 0;
    return ++time;
}

// Mutex Locks and Semaphores
pthread_mutex_t waitingAreaLock;
pthread_mutex_t tutoringAreaLock;
sem_t coordinatorWaiting;
sem_t tutorsWaiting;
sem_t studentWaiting;

// Variable to store CMD args
int maxNumberOfStudents;
int maxNumberOfTutors;
int maxNumberofChairs;
int maxNumberofHelps;
int chairsAvailable;

// Variable to store the progress 
int totalRequests = 0;
int totalStudentsTutoredNow = 0;
int totalSessions = 0;

// Student Tread
void* studentThread(void* arg) 
{
    int id = (intptr_t)arg;
    int helpReceived = 0;

    while (helpReceived < maxNumberofHelps) 
    {
        // Simulate programming
        usleep(rand() % 2000);

        // Lock and occupie chair and notify coordinator
        pthread_mutex_lock(&waitingAreaLock);
        if (chairsAvailable > 0) 
        {
            chairsAvailable--;
            printf("S: Student %d takes a seat. Empty chairs = %d\n", id, chairsAvailable);
            Student s = {id, helpReceived, getCurrentTime()};
            enqueueCoordinatorQueue(s);
            sem_post(&coordinatorWaiting);
            pthread_mutex_unlock(&waitingAreaLock);

            // Wait for tutor
            sem_wait(&studentWaiting); 
            printf("S: Student %d received help from Tutor %d.\n", id, helpReceived + 1);
            helpReceived++;

            // Lock and free the chair
            pthread_mutex_lock(&waitingAreaLock);
            chairsAvailable++;
            pthread_mutex_unlock(&waitingAreaLock);
        } 
        else 
        {
            pthread_mutex_unlock(&waitingAreaLock);
            printf("S: Student %d found no empty chair. Will try again later.\n", id);
        }
    }

    return NULL;
}

// Coordinator Thread
void* coordinatorThread(void* arg) 
{
    while (1) 
    {
        // lock and dequeue priority student
        sem_wait(&coordinatorWaiting); 
        pthread_mutex_lock(&waitingAreaLock);
        if (coordinatorQueueCount > 0) 
        {
            Student s = dequeueCoordinatorQueue();
            printf("C: Student %d with priority %d added to the queue. Waiting students now = %d. Total requests = %d\n",
                   s.id, s.priority, maxNumberofChairs-chairsAvailable, ++totalRequests);
            pthread_mutex_unlock(&waitingAreaLock);

            // lock and add to tutoring queue
            pthread_mutex_lock(&tutoringAreaLock);
            enqueueTutorQueue(s);
            pthread_mutex_unlock(&tutoringAreaLock);

            sem_post(&tutorsWaiting);
        } 
        else 
        {
            pthread_mutex_unlock(&waitingAreaLock);
        }
    }

    return NULL;
}

// Tutor Thread
void* tutorThread(void* arg) 
{
    int id = (intptr_t)arg;

    while (1) 
    {
        sem_wait(&tutorsWaiting);

        // Lock and deque priority student
        pthread_mutex_lock(&tutoringAreaLock);
        if (tutorQueueCount > 0) 
        {
            // Updating the students turtoring now
            Student s = dequeueTutorQueue();
            totalStudentsTutoredNow++;
            pthread_mutex_unlock(&tutoringAreaLock);

            printf("T: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = %d\n",
                   s.id, id, totalStudentsTutoredNow, ++totalSessions);
            usleep(200); 
            
             // Updating the students turtoring now
            pthread_mutex_lock(&tutoringAreaLock);
            totalStudentsTutoredNow--;
            pthread_mutex_unlock(&tutoringAreaLock);
            sem_post(&studentWaiting);
        }
        else 
        {
            pthread_mutex_unlock(&tutoringAreaLock);
        }
    }

    return NULL;
}

int main(int argc, char* argv[]) 
{
    // Inputs from the command lines
    maxNumberOfStudents = atoi(argv[1]);
    maxNumberOfTutors = atoi(argv[2]);
    maxNumberofChairs = atoi(argv[3]);
    maxNumberofHelps = atoi(argv[4]);
    chairsAvailable = maxNumberofChairs;

    // Initialize Semaphores
    sem_init(&coordinatorWaiting, 0, 0);
    sem_init(&tutorsWaiting, 0, 0);
    sem_init(&studentWaiting, 0, 0);
    pthread_mutex_init(&waitingAreaLock, NULL);
    pthread_mutex_init(&tutoringAreaLock, NULL);

    // Initialize Threads
    pthread_t students[maxNumberOfStudents];
    pthread_t tutors[maxNumberOfTutors];
    pthread_t coordinator;

    // Create Coordinator Thread
    pthread_create(&coordinator, NULL, coordinatorThread, NULL);

    // Create Tutor Threads
    for (int i = 0; i < maxNumberOfTutors; i++) 
    {
        pthread_create(&tutors[i], NULL, tutorThread, (void*)(intptr_t)(i + 1));
    }

    // Create Student Threads
    for (int i = 0; i < maxNumberOfStudents; i++) 
    {
        pthread_create(&students[i], NULL, studentThread, (void*)(intptr_t)(i + 1));
    }

    // Join Student Threads
    for (int i = 0; i < maxNumberOfStudents; i++) 
    {
        pthread_join(students[i], NULL);
    }

    // Clean up
    sem_destroy(&coordinatorWaiting);
    sem_destroy(&tutorsWaiting);
    sem_destroy(&studentWaiting);
    pthread_mutex_destroy(&waitingAreaLock);
    pthread_mutex_destroy(&tutoringAreaLock);

    return 0;
}