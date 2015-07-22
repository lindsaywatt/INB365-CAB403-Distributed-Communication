#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "assign2.h"

volatile int running;

int main(int argc, char *argv[]) {
    int sock, port;
    char* hostname;
    char buf[CLIENT_RECV_BUFFER];
    struct hostent *he;
    struct sockaddr_in their_addr;
    
    running = 1;
    signal(SIGINT, sigintHandlerClient);
    
    if (argc != 3) {
        fprintf(stderr,"usage: ./client <hostname> <port>\n");
        exit(1);
    } else {
        hostname = argv[1];
        port = atoi(argv[2]);
    }
    
    if ((he=gethostbyname(hostname)) == NULL) {
        perror("gethostbyname");
        exit(1);
    }
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(port);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
    
    if (connect(sock, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }
    
    // search loop
    system("clear");
    int numbytes;
    while (running) {
        if ((numbytes=recv(sock, buf, CLIENT_RECV_BUFFER, 0)) == -1) {
            perror("recv");
            break;
        }
        if (numbytes > 0) {
            // if a messsage was recieved
            buf[numbytes] = '\0';
            printf("%s",buf);
            if (strcmp(&buf[numbytes-1], ">") == 0) {
                // greater than symbol at end of string recieved from server
                // indicates that it is awaiting a reply from client.
                char* to_send = malloc(QUERY_LENGTH);
                fgets(to_send, QUERY_LENGTH, stdin);
                sscanf(to_send, "%[^\n]", to_send);
                
                send(sock, to_send, strlen(to_send)+1, 0);
                free(to_send);
            } else if (!strcmp(buf, "quit")) {
                break;
            }
        }
    }

    close(sock);
    printf("\nClosed the connection to server\n");
    
    return EXIT_SUCCESS;
}

void sigintHandlerClient(int sig_num) {
    if (sig_num == SIGINT) {
        running = 0;
        printf("\nReceived SIGINT - stopping client...\n");
    }
}
