// Spataru Florina Gabriela 323 CA // florina.spataru@gmail.com
// Tema 3 PC // TCP Multiplex

#ifndef __LIB
#define __LIB

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <iostream>

#define MAX_CLIENTS 13
#define BUFLEN 256
#define MSGLEN 1024
#define SENDLEN 5000

using namespace std;

typedef struct client {
   char name[BUFLEN];
   int port;
   int initial_time;
   char ip_adr[BUFLEN];
   int fd;
} client_type;

typedef struct my_struct1 {
   int type;
   char payload[SENDLEN];
} msg;

typedef struct my_struct2 {
   int port;
   bool sent;
   char name[BUFLEN];
   char message[MSGLEN];
} chat;

#endif