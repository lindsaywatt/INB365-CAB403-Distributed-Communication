#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h> 
#include "assign2.h"

const char* DELIMITER = ",";
char COMMENT_FLAG = '#';

/* 'ssend' function acts as an easy interface for the standard send function
 * 'int sock' - socket to send 'char* message' to
*/
void ssend(int sock, char* message) {
    size_t len = strlen(message);
    if (send(sock, (char*)message, len, 0) == -1)
        perror("send");
}

/* 'recieve' will block until a message apears on socket 'sock'
 * ptr will be populated with the message
*/
void recieve(int sock, char* ptr) {
    int numbytes;
    while (1) {
        if ((numbytes=recv(sock, ptr, QUERY_LENGTH, 0)) == -1) {
            printf("recv");
            exit(1);
        }
        if (numbytes > 0)
            break;
    }
}

/* 'recievei' will block until a message apears on socket 'sock'
 *  function will return the message as an int
*/
int recievei(int sock) {
    char ptr[10];
    int numbytes;
    while (1) {
        if ((numbytes=recv(sock, ptr, QUERY_LENGTH, 0)) == -1) {
            printf("recv");
            exit(1);
        }
        if (numbytes > 0) {
            return atoi(ptr);
        }
    }
}

/* 'readCSVFile' will read in the file 'FILENAME' and create a linked list
 * with it's root pointer stored in 'database_root'
*/
void readCSVFile() {
    FILE* csv = fopen(FILENAME, "r");
    // buffer will store one line from the file at a time
    char buffer[BUFFER_SIZE];
    food_t* cursor;
    while(fgets(buffer, BUFFER_SIZE, csv) != NULL) {
        if (buffer[0] == COMMENT_FLAG)
            // skip comment lines
            continue;
        if (database_root == NULL) {
            // first element
            database_root = lineToStruct(buffer);
            cursor = database_root;
        } else {
            cursor->next = lineToStruct(buffer);
            cursor = cursor->next;
        }
    }
    fclose(csv);
}

/* 'writeCSVFile' will write the contents of the linked list 'database_root'
 *  to the file 'FILENAME' in CSV format.
*/
void writeCSVFile() {
    FILE* csv = fopen(FILENAME, "w");
    for (food_t* cur = database_root; cur != NULL; cur = cur->next) {
        fprintf(csv, "%s,%s,%d,%d,%d,%d,%d\n",\
                cur->name, cur->measure, cur->weight,\
                cur->calories, cur->fat,\
                cur->carbohydrates, cur->protein);
    }
    fclose(csv);
}

/* 'lineToStruct' takes a line ('buffer') from the CSV file
 *  and populates a new food item linked list node.
*/
food_t* lineToStruct(char* buffer) {
    char* cpy = malloc(BUFFER_SIZE);
    memcpy(cpy, buffer, BUFFER_SIZE);
    
    food_t* ptr = (food_t*)malloc(sizeof(food_t)); // allocate memory for a new food item
    ptr->name = malloc(NAME_BUFFER);
    ptr->measure = malloc(NAME_BUFFER);

    // tokenise string
    char* token;
    token = strtok(cpy, DELIMITER);
    
    int i = 0, first = 1;
    while (token != NULL) {
        if (first || !isdigit(token[0])) {
            // First token is always part of the food name.
            // After the first token is extracted, the food name is-
            // finished when 1st char of the token is a digit.
            // The food measure always starts with a digit.
            ptr->name = strcat(strcat(ptr->name, token), " ");
            first = 0;
        } else {
            switch (i) {
                case 0: memcpy(ptr->measure, token, NAME_BUFFER);
                    break;
                case 1: ptr->weight = atoi(token);
                    break;
                case 2: ptr->calories = atoi(token);
                    break;
                case 3: ptr->fat = atoi(token);
                    break;
                case 4: ptr->carbohydrates = atoi(token);
                    break;
                case 5: ptr->protein = atoi(token);
                    break;
            }
            ++i;
        }
        token = strtok(NULL, DELIMITER);
    }
    free(cpy);
    ptr->next = NULL;
    return ptr;
}

/* 'insertFood' will ask the client for all the required fields in a food item
 *  then find the correct alphabetical position for the new item in the linked list
*/
void insertFood(int sock) {
    food_t* ptr = (food_t*)malloc(sizeof(food_t)); // allocate memory for a new food item
    ptr->name = malloc(NAME_BUFFER);
    ptr->measure = malloc(NAME_BUFFER);

    ssend(sock, "Enter information about the food below.\n\nNote:\n\t(Capitalise each word in 'Name')\n\t('Measure' must start with a number)\n\n");    
    fflush(stdin);
    ssend(sock, "Name >");
    recieve(sock, ptr->name);
    ssend(sock, "Measure >");
    recieve(sock, ptr->measure);
    ssend(sock, "Weight (g) >");
    ptr->weight = recievei(sock);
    ssend(sock, "kCal >");
    ptr->calories = recievei(sock);
    ssend(sock, "Fat (g) >");
    ptr->fat = recievei(sock);
    ssend(sock, "Carbs (g) >");
    ptr->carbohydrates = recievei(sock);
    ssend(sock, "Protein (g) >");
    ptr->protein = recievei(sock);
    
    ptr->next = NULL;
    
    pthread_mutex_lock(&database_lock); // critical section
    food_t* cursor = database_root;
    food_t* last_cur = cursor;
    for (cursor; cursor!=NULL; cursor=cursor->next) {
        if (strcmp(ptr->name, cursor->name) < 0) {
            if (cursor == last_cur)
                insertIntoLinkedList(NULL, ptr); // new food item becomes new root
            else
                insertIntoLinkedList(last_cur, ptr); // new food item goes after last_cur in list
            break;
        }
        last_cur = cursor;
    }
    pthread_mutex_unlock(&database_lock); // exit critical section
    
    ssend(sock, "\nThis was added to the database:\n\n");
    printFood(sock, ptr);
}

/* the search function takes a char* which is the query, and returns a pointer
 * to a linked list of results.
 * if no matches are found, the pointer returned is NULL.
*/
food_t* search(char* query) {   
    query = tolowers(query);
    char* food_name = malloc(NAME_BUFFER);
    int num_results = 0;

    food_t* results = (food_t*)malloc(sizeof(food_t)); // allocate memory for linked list
    food_t* r_cursor;
    
    // for each node in linked list, compare first word in name converted to lower case-
    //    with the query string converted to lowercase
    for (food_t* db_cursor=database_root; db_cursor!=NULL; db_cursor=db_cursor->next) {
        food_name = tolowers(getWordBeforeSpace(db_cursor->name));
        if (strcmp(food_name, query) == 0) {
            // found a match
            if (num_results == 0) {
                // first result popultes the root
                memcpy(results, db_cursor, sizeof(food_t));
                r_cursor = results;
            } else {
                // subsequent results go in after root
                r_cursor->next = (food_t*)malloc(sizeof(food_t));
                memcpy(r_cursor->next, db_cursor, sizeof(food_t));
                r_cursor = r_cursor->next;
            }
            ++num_results;
        }
        free(food_name);
    }
    if (num_results > 0) {
        // ensure last node's 'next' member points to NULL
        r_cursor->next = NULL;
    } else {
        // no results found
        free(results);
        results = NULL;
    }
    return results;
}

// This function returns the part of a string before the first space
char* getWordBeforeSpace(char* string) {
    char* cpy = malloc(NAME_BUFFER);
    memcpy(cpy, string, NAME_BUFFER);
    return strtok(cpy, " ");
}

// tolowers converts a string at *string to all lowercase
char* tolowers(char* string) {
    for(int i=0; string[i]; ++i)
        string[i] = tolower(string[i]);
    return string;
}

// removes and returns index tail from queue
int deleteFromQueue(queue_t* queue) {
    int val;
    val = queue->queue_i[queue->tail];
    queue->tail = (queue->tail+1) % NUM_THREADS;
    // tail is incremented to keep track of which index to remove next time
    return val;
}

// adds val to index head in queue
void addToQueue(queue_t* queue, int val) {
    queue->queue_i[queue->head] = val;
    queue->head = (queue->head+1) % NUM_THREADS;    
    // head is incremented to keep track of which index to insert val at next time
}

// formats and prints the food item pointer to the socket specified in 'sock'
void printFood(int sock, food_t* food) {
    char to_send[250];
    sprintf(to_send, "Food: %s\nMeasure: %s\nWeight: %dg\nkCal: %d\nFat: %dg\nCarbo: %dg\nProtein: %dg\n\n",\
            food->name, food->measure, food->weight, food->calories,\
            food->fat, food->carbohydrates, food->protein);
    ssend(sock, to_send);
}

// walks through a linked list of food_t*'s and prints all food items
void printLinkedList(int sock, food_t* root) {
    for (root; root != NULL; root = root->next)
        printFood(sock, root);
}

// walks through a linked list of food_t*'s and returns the length
int lengthLinkedList(food_t* root) {
    int length = 0;
    for (root; root != NULL; root = root->next) 
        ++length;
    return length;
}

// inserts the food_t* 'to insert' after the food_t* 'previous' in a linked list
// sets 'to_insert->next' to the rest of the linked list
void insertIntoLinkedList(food_t* previous, food_t* to_insert) {
    if (previous == NULL) {
        // to_insert becomes the new root
        to_insert->next = database_root;
        database_root = to_insert;  
    } else {
        food_t* temp = previous->next;
        previous->next = to_insert; 
        to_insert->next = temp;
    }
}

void freeLinkedList(food_t* root) {
    while (root != NULL) {
        food_t* temp = root;
        root = temp->next;
        free(temp);
    }
}
