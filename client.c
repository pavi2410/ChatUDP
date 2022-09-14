#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "chat.h"

int sockfd;

bool done = false;
pthread_mutex_t mutexsum = PTHREAD_MUTEX_INITIALIZER; // mutual exclusion for our threads

void *sender();
void *receiver();

char sendBuffer[BUF_SIZE];
char receiveBuffer[BUF_SIZE + USERNAME_LEN + 2];

int main(int argc, char *argv[]) {
  bzero(sendBuffer, BUF_SIZE);
  bzero(receiveBuffer, BUF_SIZE + USERNAME_LEN + 2);

  int portnum;
  char username[USERNAME_LEN];

  if (argc != 4) {
    fprintf(stderr, "Usage: %s [server ip] [server port] [your username]\n",
            argv[0]);
    exit(1);
  }

  portnum = atoi(argv[2]);
  strncpy(username, argv[3], USERNAME_LEN);

  printf("server: %s\n", argv[1]);
  printf("port: %d\n", portnum);
  printf("username: %s\n\n", username);

  // allow server to resolve hostnames or use ip's
  struct hostent *server_host;

  if ((server_host = gethostbyname(argv[1])) == NULL) { // get the host info
    perror("gethostbyname");
    exit(1);
  }

  struct sockaddr_in server_addr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    close(sockfd);
    perror("socket");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portnum);
  server_addr.sin_addr = *((struct in_addr *)server_host->h_addr);
  memset(&(server_addr.sin_zero), 0, 8);

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    close(sockfd);
    perror("connect");
    exit(1);
  }

  // Create and send out open message to the server so it knows our username and
  // we are identified as a connected client
  strcpy(sendBuffer, username);
  if (send(sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
    perror("send");
    close(sockfd);
    exit(1);
  }

  // create threads
  // Thread 1: takes in user input and sends out messages
  // Thread 2: listens for messages that are comming in from the server and
  // prints them to screen
  // Set up threads
  pthread_t threads[2];
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Run the sender and receiver threads
  pthread_create(&threads[0], &attr, sender, NULL);
  pthread_create(&threads[1], &attr, receiver, NULL);

  // Wait until done is true then exit program
  while (!done);

  close(sockfd);
  return 0;
}

void *sender() {
  while (1) {
    bzero(sendBuffer, BUF_SIZE);
    scanf("%s", sendBuffer);

    // send message to server
    if (send(sockfd, sendBuffer, strlen(sendBuffer), 0) == -1) {
      perror("send");
      done = true;
      pthread_mutex_destroy(&mutexsum);
      pthread_exit(NULL);
    }

    // Check for quiting
    if (strcmp(sendBuffer, CMD_LOGOUT) == 0) {
      done = true;
      pthread_mutex_destroy(&mutexsum);
      pthread_exit(NULL);
    }

    pthread_mutex_unlock(&mutexsum);
  }
}

void *receiver() {
  int nbytes;

  while (1) {
    bzero(receiveBuffer, BUF_SIZE);

    // Receive messages from server
    if ((nbytes = recv(sockfd, receiveBuffer, BUF_SIZE - 1, 0)) == -1) {
      perror("recv");
      done = true;
      pthread_mutex_destroy(&mutexsum);
      pthread_exit(NULL);
    }

    receiveBuffer[nbytes] = 0;
    if (strcmp(ERROR, receiveBuffer) == 0) {
      printf("Error: The username is already taken.\n");
      done = true;
      pthread_mutex_destroy(&mutexsum);
      pthread_exit(NULL);
    } else {
      printf("%s", receiveBuffer);
      pthread_mutex_unlock(&mutexsum);
    }
  }
}
