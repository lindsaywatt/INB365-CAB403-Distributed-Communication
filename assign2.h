#ifndef __ASSIGN2_H__
#define __ASSIGN2_H__

#define FILENAME "calories.csv"
#define BUFFER_SIZE 200
#define CLIENT_RECV_BUFFER 100
#define NAME_BUFFER 50
#define DATABASE_ROWS 2000
#define QUERY_LENGTH 50
#define NUM_THREADS 10
#define DEFAULT_PORT 12345
#define BACKLOG 5

// food item struct, and linked list node
typedef struct food {
    char* name;
    char* measure;
    int weight;
    int calories;
    int fat;
    int carbohydrates;
    int protein;
    struct food *next;
} food_t;

typedef struct queue_s {
    int* queue_i;
    int head;
    int tail;
} queue_t;

// function prototypes
void addToQueue(queue_t* queue, int val);
int deleteFromQueue(queue_t* queue);
void printFood(int sock, food_t* food);
void insertFood(int sock);
food_t* search(char* query);
int lengthLinkedList(food_t* food);
void printLinkedList(int sock, food_t* food);
void insertIntoLinkedList(food_t* previous, food_t* to_insert);
void freeLinkedList(food_t* food);
void readCSVFile();
void writeCSVFile();
food_t* lineToStruct(char* buffer);
char* tolowers(char* string);
char* getWordBeforeSpace(char* string);
void ssend(int sock, char* message);
void recieve(int sock, char* ptr);
int recievei(int sock);
void* worker(void* arg);
void client_function(int sock);
void sigintHandlerServer(int sig_num);
void sigintHandlerClient(int sig_num);

// global variables
food_t* database_root; // will store the root of the food linked list
queue_t* queue; // will store producer/consumer buffer
volatile int running;  // allows main thread to stop other threads
pthread_t threadpool[NUM_THREADS];
pthread_mutex_t lock, database_lock;
sem_t full, empty;

#endif
