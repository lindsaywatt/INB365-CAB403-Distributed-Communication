#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h> 
#include <sys/wait.h>
#include "assign2.h"

int main(int argc, char* argv[]) {
    running = 1;
    signal(SIGINT, sigintHandlerServer);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, NUM_THREADS);
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&database_lock, NULL);
    
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;

    int server_sock, new_sock; // listen on server_sock, new connections go in new_sock
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t sock_size = sizeof(struct sockaddr_in);

    // generate the socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // generate the end point
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket to endpoint
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    // start listnening
    if (listen(server_sock, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("\nServer is listening on port %d\n", port);

    // read csv file into memory
    readCSVFile();
    if (database_root == NULL) {
        printf("Error opening csv file\n");
        exit(1);
    }
    
    // start NUM_THREADS number of threads
    for (int i=0; i<NUM_THREADS; ++i) {
        pthread_create(&threadpool[i], NULL, worker, NULL);
        printf("thread %d started\n", i+1);
    }
    
    queue = (queue_t*)malloc(sizeof(queue_t*));
    queue->queue_i = malloc(sizeof(int)*NUM_THREADS);
    queue->head = 0;
    queue->tail = 0;
    
    // main accept loop - producer
    while(running) {
        if ((new_sock = accept(server_sock,(struct sockaddr *)&client_addr, &sock_size)) == -1) {
            // SIGINT will cause accept' to stop blocking and return -1
            printf("accept thread stopped...\n");
            break;
        } else {
            sem_wait(&empty); // wait for an emptpy spot in the queue
            pthread_mutex_lock(&lock); // go into critical section
            addToQueue(queue, new_sock);
            pthread_mutex_unlock(&lock); // leave critical section
            printf("\nnew connection from: %s\n", inet_ntoa(client_addr.sin_addr));
            sem_post(&full); // announce that items are available to consume
        }
    }

    // send cancel signal to all threads
    // and wait for them to quit
    for (int i=0; i<NUM_THREADS; ++i) {
        pthread_cancel(threadpool[i]);
        pthread_join(threadpool[i], NULL);
        printf("thread %d stopped...\n", i+1);
    }

    sem_destroy(&full);
    sem_destroy(&empty);
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&database_lock);
    close(server_sock);
    writeCSVFile(); // write CSV with new additions to the file system
    freeLinkedList(database_root);
    free(queue->queue_i);
    free(queue);
    return EXIT_SUCCESS;

} // END main

// threadpool worker thread
void* worker(void* arg) {
    while (running) {
        sem_wait(&full); // wait for a new connection to consume
        pthread_mutex_lock(&lock); // critical section
        int socket = deleteFromQueue(queue);    
        pthread_mutex_unlock(&lock);
        sem_post(&empty); // announce that a space is available in queue
        client_function(socket); // launch function that will interact with the client
    }
    return NULL;
}

void client_function(int sock) {
    food_t* results = NULL;
    while (running) {
        ssend(sock, "\nEnter the food name to search, ‘a’ to add a new food item, or ‘q’ to quit:\n>");
        char query[QUERY_LENGTH];
        recieve(sock, query); // block until something recieved from client

        if (!strcmp(query,"q")) { // quit indicator was recieved
            ssend(sock, "quit");
            break;
        } else if (!strcmp(query,"a")) { // insert food indicatior was recieved
            insertFood(sock);
        } else if (running) {
            pthread_mutex_lock(&database_lock); // critical section
            results = search(query);
            pthread_mutex_unlock(&database_lock);
            if (results == NULL) {
                // food items were not found
                ssend(sock, "\nNo food item found.\nPlease check your spelling and try again.\n");
            } else {
                // food items were found
                int num_results = lengthLinkedList(results);
                char message[NAME_BUFFER];
                sprintf(message, "\n%d food items were found.\n\n", num_results);
                ssend(sock, message);
                printLinkedList(sock, results);
                freeLinkedList(results);
            }
        }
    }       
    close(sock);
    printf("socket %d closed\n", sock);
}

void sigintHandlerServer(int sig_num) {
    if (sig_num == SIGINT) {
        running = 0;
        printf("\nReceived SIGINT - stopping server...\n");
    }
}
