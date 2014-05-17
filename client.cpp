// Spataru Florina Gabriela 323 CA // florina.spataru@gmail.com
// Tema 3 PC // TCP Multiplex

#include "lib.h"

/* Type of messages sent from client:
 *      listclients                     23
 *      infoclient <client_name>        24 
 *      message <client_name> <message> 25
 *      broadcast <message>             26
 *      sendfile <dest_client> <file>   27
 */

/* Message types
 *      accept                          42
 *      kick/reject                     100
 *      no such client                  113
 */

using namespace std;

int main(int argc, char *argv[]){

   if (argc < 5) {
      fprintf(stderr,"Usage %s client_name client_port server_ip server_port\n", argv[0]);
      return -1;
   }

   int server_sockfd, admin_sockfd, newsockfd, n;
   struct sockaddr_in serv_addr, admin_addr, cli_addr;
   struct hostent *server;
   int clilen;
   int i, j;
   int fdmax;
   bool pass;

   char buffer[BUFLEN];
   char param1[BUFLEN];
   char param2[BUFLEN];
   char auxiliary[SENDLEN];
   vector<chat> msg_in_progress;
   vector<client_type> client_list;

   time_t current_time;
   time_t now = time(NULL);
   struct tm* timeInfo = localtime(&now);
   client_type client, dummy_client;
   msg t;
   chat p;

   fd_set read_fds, tmp_fds;
   FD_ZERO(&read_fds);
   FD_ZERO(&tmp_fds);

   // Client info
   strcpy(client.name, argv[1]);
   strcpy(client.ip_adr, argv[3]);
   client.port = atoi(argv[2]);

   // Socket for connection with server
   server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (server_sockfd < 0){
      fprintf(stderr, "ERROR opening socket!\n");
      return -1;
   }

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port   = htons(atoi(argv[4]));
   inet_aton(argv[3], &serv_addr.sin_addr);

   // Connect to server
   if (connect(server_sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0){
      fprintf(stderr, "ERROR connecting\n");
      return -1;
   }
   // Admin socket
   admin_sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (admin_sockfd < 0){
      fprintf(stderr, "ERROR opening socket!\n");
      return -1;
   }

   admin_addr.sin_family        = AF_INET;
   admin_addr.sin_addr.s_addr   = INADDR_ANY;
   admin_addr.sin_port          = htons(client.port);

   if(bind(admin_sockfd, (struct sockaddr *) &admin_addr,sizeof(struct sockaddr)) < 0){
      fprintf(stderr, "ERROR on binding\n");
      return -1;
   }

   listen(admin_sockfd, MAX_CLIENTS);

   FD_SET(server_sockfd, &read_fds);
   FD_SET(admin_sockfd, &read_fds);
   FD_SET(0, &read_fds);
   fdmax = admin_sockfd;

   // Sending my info to server
   memset(auxiliary, 0, SENDLEN);
   memcpy(auxiliary, &client, sizeof(client_type));
   n = send(server_sockfd, auxiliary, sizeof(client_type), 0);
   if (n < 0){
      fprintf(stderr, "ERROR writing to socket\n");
      return -1;
   }

   memset(auxiliary, 0, SENDLEN);
   memset(t.payload, 0, SENDLEN);

   n = recv(server_sockfd, auxiliary, SENDLEN, 0);
   t = *((msg *) auxiliary);

   // Check answer
   if (t.type == 100){
      fprintf(stderr, "I've been rejected :(\n");
      close(server_sockfd);
      close(admin_sockfd);
      return -1;
   }
   else
      printf("Connected!\n");

   while(1){
      // Must check if I have messages in progress
      for (i = 0; i < msg_in_progress.size(); i++){
         if (msg_in_progress[i].sent == false && msg_in_progress[i].port != 0){
            // Message ready to be send
            pass = false;
            for (j = 0; j < client_list.size(); j++){
               if (strcmp(client_list[j].name, msg_in_progress[i].name) == 0){
                  pass = true;
                  break;
               }
            }
            if (pass == false){ // No connection to this client
               newsockfd = socket(AF_INET, SOCK_STREAM, 0);
               serv_addr.sin_family         = AF_INET;
               serv_addr.sin_addr.s_addr    = INADDR_ANY;
               serv_addr.sin_port           = htons(msg_in_progress[i].port);
               printf("New connection with %s on port %d\n",
                      msg_in_progress[i].name, msg_in_progress[i].port);
               if (connect(newsockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0){
                  fprintf(stderr, "ERROR connecting\n");
                  return -1;
               }

               strcpy(dummy_client.name, msg_in_progress[i].name);
               dummy_client.port = msg_in_progress[i].port;
               strcpy(dummy_client.ip_adr, "127.0.0.1");
               dummy_client.fd = newsockfd;
               client_list.push_back(dummy_client);

               // Sending my information to the other client
               memset(t.payload, 0, SENDLEN);
               t.type = 42;
               memcpy(t.payload, &client, sizeof(client_type));
               n = send(newsockfd, &t, sizeof(msg), 0);
               if (n < 0){
                  fprintf(stderr, "ERROR writing to socket\n");
                  return -1;
               }

               // Sending the message *chatting*
               FD_SET(newsockfd, &read_fds);
               if (newsockfd > fdmax)
                  fdmax = newsockfd;
               memset(t.payload, 0, SENDLEN);
               strcpy(t.payload, msg_in_progress[i].message);
               t.type = 25;
               n = send(newsockfd, &t, sizeof(msg), 0);
               msg_in_progress[i].sent = true;
               if (n < 0){
                  fprintf(stderr, "ERROR writing to socket\n");
                  return -1;
               }
            }
            else { // Already connected
               memset(t.payload, 0, SENDLEN);
               strcpy(t.payload, msg_in_progress[i].message);
               t.type = 25;
               n = send(client_list[j].fd, &t, sizeof(msg), 0);
               msg_in_progress[i].sent = true;
               if (n < 0){
                  fprintf(stderr, "ERROR writing to socket\n");
                  return -1;
               }
            }
         }
      }

      while (1){
         pass = false;
         for (i = 0; i < msg_in_progress.size(); i++){
            if (msg_in_progress[i].sent == true){
               pass = true;
               break;
            }
         }
         if (pass == true){
            msg_in_progress.erase(msg_in_progress.begin() + i);
         }
         else
            break;
      }
      
      tmp_fds = read_fds; 
      if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1){
         fprintf(stderr, "ERROR in select\n");
         return -1;
      }

      for(i = 0; i <= fdmax; i++) {
         if (FD_ISSET(i, &tmp_fds)) {

            if (i == 0){ // Client: Imma writing!
               memset(buffer, 0 , BUFLEN);
               scanf("%s", buffer);

               if (strcmp(buffer, "listclients") == 0){
                     memset(t.payload, 0, SENDLEN);
                     t.type = 23;

                     // Sending message 23 == listclients to server
                     n = send(server_sockfd, &t, sizeof(int), 0);
                     if (n < 0){
                        fprintf(stderr, "ERROR writing to socket\n");
                        return -1;
                     }
               }

               if (strcmp(buffer, "infoclient") == 0){
                  memset(t.payload, 0, SENDLEN);
                  t.type = 24;
                  scanf("%s", t.payload);

                  n = send(server_sockfd, &t, strlen(t.payload) + sizeof(int), 0);
                  if (n < 0){
                     fprintf(stderr, "ERROR writing to socket\n");
                     return -1;
                  }
               }

               if (strcmp(buffer, "message") == 0){
                  memset(t.payload, 0, SENDLEN);
                  memset(p.message, 0, MSGLEN);
                  memset(p.name, 0, BUFLEN);
                  t.type = 25;
                  scanf("%s", p.name);
                  fgets(p.message, 100, stdin);
                  p.port = 0;
                  p.sent = false;

                  msg_in_progress.push_back(p);

                  strcpy(t.payload, p.name);
                  n = send(server_sockfd, &t, strlen(t.payload) + sizeof(int), 0);
                  if (n < 0){
                     fprintf(stderr, "ERROR writing to socket\n");
                     return -1;
                  }
               }

               if (strcmp(buffer, "quit") == 0){
                  memset(t.payload, 0, SENDLEN);
                  t.type = 28;
                  
                  for (j = 0; j <= fdmax; j++){
                     if (FD_ISSET(j, &read_fds)){
                        n = send(j, &t, sizeof(int), 0);
                        close(j);
                        FD_CLR(j, &read_fds);
                     }
                  }
                  close(admin_sockfd);
               }

            }

            else if (i == admin_sockfd) {
               // New connection with some client
               clilen = sizeof(cli_addr);
               if ((newsockfd = accept(admin_sockfd, (struct sockaddr *)&cli_addr,(socklen_t*) &clilen)) == -1) {
                  fprintf(stderr, "ERROR in accept");
                  return -1;
               } 
               else {
                  if ((n = recv(newsockfd, auxiliary, sizeof(auxiliary), 0)) <= 0) {
                     if (n != 0) {
                        fprintf(stderr, "ERROR in recv\n");
                        return -1;
                     }
                  }
                  else{
                     t = *((msg *) auxiliary);
                     dummy_client = *((client_type *) t.payload);
                     dummy_client.fd = newsockfd;
                     client_list.push_back(dummy_client);
                     printf("New connection with %s on port %d\n",
                        dummy_client.name, dummy_client.port);
                  }

                  FD_SET(newsockfd, &read_fds);
                  if (newsockfd > fdmax) { 
                     fdmax = newsockfd;
                  }
               }
            }

            else if (i == server_sockfd){
               // Getting data from server
               memset(auxiliary, 0, SENDLEN);
               if ((n = recv(i, auxiliary, sizeof(auxiliary), 0)) <= 0) {
                  if (n != 0) {
                     fprintf(stderr, "ERROR in recv\n");
                     return -1;
                  }
                  printf("Server performed seppuku\n");
                  printf("Doing so myself...\n");
                  for (j = 0; j <= fdmax; j++){
                     if (FD_ISSET(j, &read_fds)){
                        close(j);
                        FD_CLR(j, &read_fds);
                     }
                  }
                  close(admin_sockfd);
                  return -1;
               }
               else {
                  // I'm getting data from server
                  t = *((msg *) auxiliary);

                  switch(t.type){
                     case 100: { // kick
                        printf("Server kicked me...\n");
                        for (j = 0; j <= fdmax; j++){
                           if (FD_ISSET(j, &read_fds)){
                              close(j);
                              FD_CLR(j, &read_fds);
                           }
                        }
                        close(admin_sockfd);
                        return -1;
                     }
                     case 23: { // listclients
                        printf("Other clients:\n");
                        printf("%s", t.payload);

                        break;
                     }
                     case 24: { // infoclient <client_name>
                        dummy_client = *((client_type *) t.payload);
                        printf("Client %s\tport %d\ttime elapsed %ds\n",
                              dummy_client.name, dummy_client.port,
                               (int)time(&current_time) - dummy_client.initial_time);

                        break;
                     }
                     case 113: { // no such client
                        printf("No such client mon!\n");
                        break;
                     }
                     case 25: { // message
                        dummy_client = *((client_type *) t.payload);
                        for (j = 0; j < msg_in_progress.size(); j++){
                           if (strcmp(dummy_client.name, msg_in_progress[j].name) == 0){
                              msg_in_progress[j].port = dummy_client.port;
                           }
                        }
                        break;
                     }
                  }
               }
            }
            else {
               // Data from other clients o.O
               memset(auxiliary, 0, sizeof(auxiliary));
               if ((n = recv(i, auxiliary, sizeof(auxiliary), 0)) <= 0) {
                  if (n != 0) {
                     fprintf(stderr, "ERROR in recv\n");
                     return -1;
                  }
                  
                  for (j = 0; j < client_list.size(); j++){
                     if (client_list[j].fd == i){
                        printf("Client %s died\n", client_list[j].name);
                        break;
                     }
                  }
                  FD_CLR(client_list[j].fd, &read_fds);
                  close(client_list[j].fd);
                  client_list.erase(client_list.begin() + j);
               }
               else{
                  t = *((msg *) auxiliary);
                  switch (t.type){
                     case 25: {
                        for (j = 0; j < client_list.size(); j++){
                           if (client_list[j].fd == i){
                              printf("[%d:%d:%d][%s] %s\n", timeInfo->tm_hour,
                                    timeInfo->tm_min, timeInfo->tm_sec,
                                    client_list[j].name, t.payload);
                              break;
                           }
                        }
                        break;
                     }
                  }
               }
            }
         }
      }
   }

   return 0;
}
