#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <vector>
// Define constants
#define e 100
#define c 500
#define k 10
#define s 20
#define MAX 5
#define T 1

using namespace std;

// Define a structure to represent each query
struct query {
    int event_num;
    int query_type;
    int thread_num;
};

// Declare a shared table of MAX entries
struct query shared_table[MAX];

int seats[e] = {0};

pthread_mutex_t shared_table_lock;
pthread_mutex_t seats_table_lock;
sem_t max_no_of_queries;



// Define a function to generate a random query type (0=read, 1=write, 2=cancel)
int random_query_type() {
    return rand() % 3;
}

// Define a function to generate a random event number (0 to e-1)
int random_event_num() {
    return rand() % e;
}

// Define a function to generate a random seat number (0 to c-1)
int random_seat_num() {
    return rand() % k + 5;
}

// Printing table
void printTable() {
    pthread_mutex_lock(&shared_table_lock);

    printf("\n\nTable:\n");
    printf(" -----------------------------------\n");
    printf("| Event No | Query Type | Thread ID |\n");
    printf("|-----------------------------------|\n");
    for(int i = 0; i < MAX; i++) {
        printf("|      %3d |         %2d |       %3d |\n", shared_table[i].event_num, shared_table[i].query_type, shared_table[i].thread_num);
    }
    printf(" -----------------------------------\n");

    int val;
    sem_getvalue(&max_no_of_queries, &val);
    printf("Semaphore : %d\n", val);
    printf("\n");

    pthread_mutex_unlock(&shared_table_lock);
}

// printing seats
void printSeats() {
    pthread_mutex_lock(&seats_table_lock);

    printf(" ------------------------\n");
    printf("| Event No | No of Seats |\n");
    printf("|------------------------|\n");
    for(int i = 0; i < e; i++) {
        printf("|      %3d |         %3d |\n", i, seats[i]);
    }
    printf(" ------------------------\n\n");

    pthread_mutex_unlock(&seats_table_lock);
}

// Removing Query from table
bool removeQuery(int event_num, int tid) {
    pthread_mutex_lock(&shared_table_lock);
    for(int i = 0; i < MAX; i++) {
        if(shared_table[i].event_num == event_num && shared_table[i].thread_num == tid) {
            // unlock
            shared_table[i].event_num = -1;
            shared_table[i].query_type = -1;
            shared_table[i].thread_num = -1;
            sem_post(&max_no_of_queries);
            pthread_mutex_unlock(&shared_table_lock);
            return true;
        }
    }
    // unlock
    pthread_mutex_unlock(&shared_table_lock);
    return false;
}


// Define a function to simulate a read query
int do_read_query(int event_num, int thread_num) {
    int ans = 0;
    // Acquire the corresponding mutex
    pthread_mutex_lock(&seats_table_lock);

    // Simulate some work
    ans = seats[event_num];
    // Release the mutex
    pthread_mutex_unlock(&seats_table_lock);

    removeQuery(event_num, thread_num);

    // Print a message indicating success
    // printf("Thread %d read event %d\n", thread_num, event_num);
    return ans;
}

// Define a function to simulate a write query
bool do_write_query(int event_num, int thread_num, int no_of_seats) {
    bool rv = true;
    // Acquire the corresponding mutex
    pthread_mutex_lock(&seats_table_lock);

    if(seats[event_num] + no_of_seats <= c) 
        seats[event_num] += no_of_seats;
    else rv =  false;
    // Release the mutex
    pthread_mutex_unlock(&seats_table_lock);

    removeQuery(event_num, thread_num);
    // Print a message indicating success
    // printf("Thread %d wrote to event %d\n", thread_num, event_num);
    return rv;
}

// Define a function to simulate a cancellation request
bool do_cancel_query(int event_num, int thread_num, int no_of_seats) {
    bool rv = true;
    // Acquire the corresponding mutex
    pthread_mutex_lock(&seats_table_lock);

    if(seats[event_num] - no_of_seats >= 0)
        seats[event_num] -= no_of_seats;
    else rv = false;
    // Release the mutex
    pthread_mutex_unlock(&seats_table_lock);

    removeQuery(event_num, thread_num);
    // Print a message indicating success
    // printf("Thread %d wrote to event %d\n", thread_num, event_num);
    return rv;
}

bool addReadQuery(int event_num, int tid) {
    // usleep(350000);
    // lock
    pthread_mutex_lock(&shared_table_lock);
    for(int i = 0; i < MAX; i++) {
        if(shared_table[i].event_num == event_num && shared_table[i].query_type != 0) {
            // unlock
            pthread_mutex_unlock(&shared_table_lock);
            return false;
        }
    }
    // unlock
    pthread_mutex_unlock(&shared_table_lock);
    
    //wait()
    sem_wait(&max_no_of_queries);
    
    //lock
    pthread_mutex_lock(&shared_table_lock);
    for(int i = 0; i < MAX; i++) {
        if(shared_table[i].event_num == -1) {
            shared_table[i].event_num = event_num;
            shared_table[i].query_type = 0;
            shared_table[i].thread_num = tid;  
            // unlock;
            pthread_mutex_unlock(&shared_table_lock);
            return true;
        }
    }
    sem_post(&max_no_of_queries);
    pthread_mutex_unlock(&shared_table_lock);
    return false;
}

bool addWriteQuery(int event_num, int query_type, int tid) {
    // lock;
    // usleep(350000);
    pthread_mutex_lock(&shared_table_lock);
    // printf("TH%d : Lock Aquired!\n", tid);

    for(int i = 0; i < MAX; i++) {
        if(shared_table[i].event_num == event_num) {
            // printf("TH%d : {event_num : %d already present!}\n", tid, event_num);
            pthread_mutex_unlock(&shared_table_lock);
            return false;
        }
    }
    pthread_mutex_unlock(&shared_table_lock);
    //wait();
    sem_wait(&max_no_of_queries);
    
    // printf("Semaphore Entered : %d\n", val);
    pthread_mutex_lock(&shared_table_lock);
    // printf("TH%d : Lock Aquired Again!\n", tid);

    for(int i = 0; i < MAX; i++) {
        if(shared_table[i].event_num == -1) {
            shared_table[i].event_num = event_num;
            shared_table[i].query_type = query_type;
            shared_table[i].thread_num = tid;
            // printf("TH%d : {event_num : %d inserted!}\n", tid, event_num);
            pthread_mutex_unlock(&shared_table_lock);
            return true;
        }
    }
    sem_post(&max_no_of_queries);
            
    pthread_mutex_unlock(&shared_table_lock);
    return false;
}

// Define a function to generate a random query and execute it
void *do_random_query(void* arg) {
    time_t begin = time(NULL);
    time_t end = time(NULL);
    int thread_num = *(int*)arg;
    printf("Thread%d is created!\n", thread_num);
    vector<vector<int>> queries; 
    while(difftime(end, begin) < T*15.0) {
        // Generate a random query type and event number
        // printf("Time TH%d : %lf\n", thread_num, difftime(end, begin));
        int query_type = random_query_type();
        int event_num = random_event_num();
        int no_of_seats = random_seat_num();
        event_num = thread_num;
        
        if(query_type == 2 && queries.size() == 0) continue;

        if(query_type == 2) {
            int ind = rand()%queries.size();
            event_num = queries[ind][0];
            no_of_seats = queries[ind][1]; 
            queries.erase(queries.begin() + ind);
        }
        
        // printf("Query Generated by Thread%d: \n", thread_num);
        // printf("{Event : %d, QType : %d, nSeats : %d}\n", event_num, query_type, no_of_seats);
        // wait until (Read)query entry is added in table
        while(query_type == 0 && !addReadQuery(event_num, thread_num));
            //wait();

        // wait until (Add, Delete)query entry is added in table
        while((query_type == 1 || query_type == 2) && !addWriteQuery(event_num, query_type, thread_num));
            //wait();
        
        
        usleep(35000);

        printTable();
        // printf("Adding Done!\n");
        
        // Complete Query added in table. 
        switch (query_type)
        {
        // Read Query
        case 0:{
            int ans = do_read_query(event_num, thread_num);
            printf("th%2d : {E%2d, QT : READ, N : %2d}\n", event_num, thread_num, ans);
            printf("Read Query is done by thread %d\n", thread_num);
            }
            break;
        // Add Query
        case 1:{
            bool success = do_write_query(event_num, thread_num, no_of_seats);
            if(!success) printf("Not successfull ==> ");
            printf("th%2d : {E%2d, QT :  ADD, N : %2d}\n", thread_num, event_num, no_of_seats); 
            if(success) queries.push_back(vector<int>{event_num, no_of_seats});
            }
            break;
        //Cancel Query
        case 2: {
            bool success = do_cancel_query(event_num, thread_num, no_of_seats);
            if(!success) printf("Not successfull ==> ");
            printf("th%2d : {E%2d, QT : CANL, N : %2d}\n", thread_num, event_num, no_of_seats);
            }
            break;
        default:
            break;
        }
        printTable();
        usleep(3500000);
        end = time(NULL);
    }
    free(arg);
    return NULL;
}


void* printTableThread(void * arg) {
    time_t begin = time(NULL);
    time_t end = time(NULL);
    // int thread_num = *(int*)arg;

    while(difftime(end, begin) < T*20.0) {
        pthread_mutex_lock(&shared_table_lock);

        printf("\n\nTable:\n");
        for(int i = 0; i < MAX; i++) {
            printf("EV: %2d, QT : %2d, Tid : %2d\n", shared_table[i].event_num, shared_table[i].query_type, shared_table[i].thread_num);
        }

        int val;
        sem_getvalue(&max_no_of_queries, &val);
        printf("Semaphore : %d\n", val);
        printf("\n\n");
        pthread_mutex_unlock(&shared_table_lock);
        
        usleep(950000);
        end = time(NULL);
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    /* code */
    sem_init(&max_no_of_queries, 0, MAX);
    pthread_t th[s];
    
    // Initializing the table
    for(int i = 0; i < MAX; i++) {
        shared_table[i].event_num = -1;
        shared_table[i].query_type = -1;
        shared_table[i].thread_num = -1;
    }

    //Creating threads
    for (int i = 0; i < s; i++) {
        int* a = new int;
        *a = i;
        if (pthread_create(&th[i], NULL, &do_random_query, a) != 0) {
            perror("Failed to created thread");
        }
        // printf("Thread%d created!\n", i);
    }
    
    // Waiting for threads to complete
    for (int i = 0; i < s; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }
    
    printf("\nFinal seat status : \n");
    printSeats();
    sem_destroy(&max_no_of_queries);
    return 0;
}
