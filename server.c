#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "chat.h"

typedef struct client {
  struct sockaddr_in address;
  char username[USERNAME_LEN];
  struct client *next;
} client;

int sockfd;

client clientList;
char requestBuffer[BUF_SIZE];
char responseBuffer[BUF_SIZE + USERNAME_LEN];

char sender_name[USERNAME_LEN];

char *colors[NCOLORS] = {RED,  GREEN,  YELLOW,  BLUE,  MAGENTA,  CYAN,
                         LRED, LGREEN, LYELLOW, LBLUE, LMAGENTA, LCYAN};

void userColor(int n) {
  strcat(responseBuffer, colors[(n % NCOLORS)]);
}

bool clientCompare(struct sockaddr_in, struct sockaddr_in);
void broadcast(struct sockaddr_in, bool);
bool isConnected(struct sockaddr_in);
bool connectClient(struct sockaddr_in, char*);
bool disconnectClient(struct sockaddr_in);
void printClientList();
void sendClientList(struct sockaddr_in);

int main(int argc, char *argv[]) {

  int server_port, nbytes;
  int address_size = sizeof(struct sockaddr_in);
  struct sockaddr_in server_addr;
  struct sockaddr_in sender_addr;

  bzero(requestBuffer, BUF_SIZE);
  bzero(responseBuffer, BUF_SIZE + USERNAME_LEN);

  if (argc != 2) {
    fprintf(stderr, "Usage: %s [port]\n", argv[0]);
    exit(1);
  }

  server_port = atoi(argv[1]);

  printf("Server running at port %d\n", server_port);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    close(sockfd);
    perror("socket");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&(server_addr.sin_zero), 0, 8);

  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    close(sockfd);
    perror("bind");
    exit(1);
  }

  while (1) {
    bzero(responseBuffer, BUF_SIZE + USERNAME_LEN);

    if ((nbytes = recvfrom(sockfd, requestBuffer, BUF_SIZE - 1, 0,
                           (struct sockaddr *)&sender_addr,
                           (unsigned int *)&address_size)) == -1) {
      perror("recvfrom");
      close(sockfd);
      exit(1);
    }

    requestBuffer[nbytes] = 0;
    printf("Received %d bytes [%s]\n", nbytes, requestBuffer);

    if (isConnected(sender_addr)) {
      strcat(responseBuffer, sender_name);
      if (strcmp(CMD_LOGOUT, requestBuffer) == 0) {
        if (disconnectClient(sender_addr)) {
          strcat(responseBuffer, RED " disconnected" RESET "\n");

          broadcast(sender_addr, true);
        }
        printClientList();
      } else if (strcmp(CMD_LIST, requestBuffer) == 0) {
        sendClientList(sender_addr);
      } else {
        strcat(responseBuffer, USERNAMExMESSAGE);
        strcat(responseBuffer, requestBuffer);
        
        printf("Message: {%s}\n", responseBuffer);
        strcat(responseBuffer, "\n");
        broadcast(sender_addr, false);
      }
    } else {
      if (connectClient(sender_addr, requestBuffer)) {
        userColor(sender_addr.sin_port);
        strcat(responseBuffer, requestBuffer);
        strcat(responseBuffer, GREEN " connected" RESET "\n");
        broadcast(sender_addr, true);
        sendClientList(sender_addr);
      }
      printClientList();
    }
  }

  close(sockfd);
  return 0;
}

bool clientCompare(struct sockaddr_in client1, struct sockaddr_in client2) {
  if (strncmp((char *)&client1.sin_addr.s_addr,
              (char *)&client2.sin_addr.s_addr, sizeof(unsigned long)) == 0) {
    if (strncmp((char *)&client1.sin_port, (char *)&client2.sin_port,
                sizeof(unsigned short)) == 0) {
      if (strncmp((char *)&client1.sin_family, (char *)&client2.sin_family,
                  sizeof(unsigned short)) == 0) {
        return true;
      }
    }
  }
  return false;
}

/* sends message to all clients except for the sender */
/* will send to all clients if global = true */
void broadcast(struct sockaddr_in sender, bool global) {
  client *cli = clientList.next;

  while (cli != NULL) {
    if (clientCompare(sender, cli->address) == false || global) {
      printf("Sending message to %s\n", cli->username);
      if ((sendto(sockfd, responseBuffer, strlen(responseBuffer), 0,
                  (struct sockaddr *)&cli->address, sizeof(struct sockaddr))) == -1) {
        perror("sendto");
        close(sockfd);
        exit(1);
      }
    }

    cli = cli->next;
  }
}

bool isConnected(struct sockaddr_in newClient) {
  client *cur_client = &clientList;

  while (cur_client != NULL) {
    if (clientCompare(cur_client->address, newClient)) {
      strncpy(sender_name, cur_client->username, USERNAME_LEN);
      printf("Client is already connected\n");
      return true;
    }
    cur_client = cur_client->next;
  }
  printf("Client is not already connected\n");
  return false;
}

bool connectClient(struct sockaddr_in newClient, char *username) {
  printf("Attempting to connect client: %s...\n", username);
  client *cur_client = &clientList;

  while (cur_client != NULL) {
    if (strcmp(cur_client->username, username) == 0) {
      printf("Cannot connect client user already exists\n");
      strcpy(responseBuffer, "");
      strcat(responseBuffer, ERROR);
      if ((sendto(sockfd, responseBuffer, strlen(responseBuffer), 0,
                  (struct sockaddr *)&newClient, sizeof(struct sockaddr))) ==
          -1) {
        perror("sendto");
        close(sockfd);
        exit(1);
      }
      return false;
    }
    cur_client = cur_client->next;
  }

  cur_client = &clientList;
  while (cur_client->next) {
    cur_client = cur_client->next;
  }
  cur_client->next = malloc(sizeof(client));
  cur_client = cur_client->next;

  cur_client->address = newClient;
  strncpy(cur_client->username, username, USERNAME_LEN);
  cur_client->next = NULL;
  printf("Client connected\n");
  return true;
}

bool disconnectClient(struct sockaddr_in oldClient) {
  printf("Attempting to disconnect client...\n");
  client *temp;
  client *cur_client = &clientList;

  while (cur_client->next != NULL) {
    if (clientCompare(oldClient, cur_client->next->address)) {
      temp = cur_client->next->next;
      free(cur_client->next);
      cur_client->next = temp;
      printf("Client disconnected\n");
      return true;
    }
    cur_client = cur_client->next;
  }

  printf("Client was not disconnected properly\n");
  return false;
}

void printClientList() {
  client *cli = clientList.next;
  int count = 1;

  while (cli != NULL) {
    printf("Client %d\n", count);
    printf("username: %s\n", cli->username);
    printf("ip: %s\n", inet_ntoa(cli->address.sin_addr));
    printf("port: %d\n\n", ntohs(cli->address.sin_port));
    cli = cli->next;
    count++;
  }
}

void sendClientList(struct sockaddr_in sender) {
  client *cli = clientList.next;
  int count = 1;
  while (cli != NULL) {
    if (clientCompare(sender, cli->address) == false) {
      strcpy(responseBuffer, "");
      char buf[5];
      sprintf(buf, "[%d] ", count++);
      strcat(responseBuffer, buf);
      userColor(cli->address.sin_port);
      strcat(responseBuffer, cli->username);
      strcat(responseBuffer, RESET "\n");
      if ((sendto(sockfd, responseBuffer, strlen(responseBuffer), 0,
                  (struct sockaddr *)&sender, sizeof(struct sockaddr))) ==
          -1) {
        perror("send");
        close(sockfd);
        exit(1);
      }
    }
    cli = cli->next;
  }
}
