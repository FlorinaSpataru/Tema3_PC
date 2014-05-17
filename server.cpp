// Spataru Florina Gabriela 323 CA // florina.spataru@gmail.com
// Tema 3 PC // TCP Multiplex

#include "lib.h"

/* Type of messages sent to client:
 * 
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

int kick (int sockfd){
   msg t;
   int n;
   
   memset(t.payload, 0, SENDLEN);
   t.type = 100; // Kick

   n = send(sockfd, &t, sizeof(int), 0);
   if (n < 0){
      fprintf(stderr, "ERROR writing to socket\n");
      return -1;
   }

   return 0;
}

int main(int argc, char *argv[]){

   if (argc < 2) {
      fprintf(stderr, "Usage : %s server_port\n", argv[0]);
      return -1;
   }

   int sockfd, newsockfd, portno;
   socklen_t clilen = sizeof(struct sockaddr_in);
   char buffer[BUFLEN]; // Small buffer
   char param[BUFLEN]; // Extra parameter in kick <client_name>
   char auxiliary[SENDLEN]; // Big buffer
   struct sockaddr_in serv_addr, cli_addr;
   int n, i, j;
   bool pass;
   int fdmax;
   time_t current_time;

   vector<client_type> client_list;
   client_type dummy_client;

   msg t;

   fd_set read_fds, tmp_fds;
   FD_ZERO(&read_fds);
   FD_ZERO(&tmp_fds);

   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0){
      fprintf(stderr, "ERROR opening socket!\n");
      return -1;
   }

   portno = atoi(argv[1]);

   memset((char *) &serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family         = AF_INET;
   serv_addr.sin_addr.s_addr    = INADDR_ANY;
   serv_addr.sin_port           = htons(portno);

   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0){
      fprintf(stderr, "ERROR on binding\n");
      return -1;
   }

   FD_SET(sockfd, &read_fds);
   FD_SET(0, &read_fds);
   fdmax = sockfd;

   listen(sockfd, MAX_CLIENTS);

   // main loop
   while (1) {
      tmp_fds = read_fds;
      if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1){
         fprintf(stderr, "ERROR in select\n");
         return -1;
      }

      for(i = 0; i <= fdmax; i++) {
         if (FD_ISSET(i, &tmp_fds)) {

            if (i == sockfd) {
               printf("New connection...\n");
               // New connection
               clilen = sizeof(cli_addr);
               if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
                  fprintf(stderr, "ERROR in accept\n");
                  return -1;
               }
               else {
                  pass = true;
                  memset(auxiliary, 0, SENDLEN);
                  n = recv(newsockfd, auxiliary, sizeof(client_type), 0);
                  dummy_client = *((client_type *) auxiliary);

                  // Checking for duplicate name
                  for (j = 0; j < client_list.size(); j++){
                     if (strcmp(client_list[j].name, dummy_client.name) == 0)
                        pass = false;
                  }

                  if (pass == true){
                     memset(t.payload, 0, SENDLEN);
                     t.type = 42;

                     // Sending an 'accept'
                     n = send(newsockfd, &t, sizeof(int), 0);
                     if (n < 0){
                        fprintf(stderr, "ERROR writing to socket\n");
                        return -1;
                     }

                     // Adding to client list
                     dummy_client.fd            = newsockfd;
                     dummy_client.initial_time  = (int)time(&current_time);
                     client_list.push_back(dummy_client);

                     // Adding socket to listening list
                     FD_SET(newsockfd, &read_fds);
                     if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                     }
                     printf("Connection to client %s\tport %d\n", dummy_client.name, dummy_client.port);
                  }
                  else{
                     memset(t.payload, 0, SENDLEN);
                     t.type = 88;

                     // Sending a 'reject'
                     kick(newsockfd);
                     close(newsockfd);
                     printf("Rejected.\n");
                  }
               }
            }

            else if (i == 0){ // Command in server
               memset(buffer, 0 , BUFLEN);
               fflush(stdin);
               scanf("%s", buffer);

               if (strcmp(buffer, "status") == 0){
                  for (j = 0; j < client_list.size(); j++){
                     printf("Client %d: %s\t%s\t%d\n", (j + 1), client_list[j].name,
                            client_list[j].ip_adr, client_list[j].port);
                  }
               }

               if (strcmp(buffer, "kick") == 0){
                  scanf("%s", param);
                  pass = false;
                  for (j = 0; j < client_list.size(); j++){
                     if (strcmp(param, client_list[j].name) == 0){
                        pass = true;
                        break;
                     }
                  }
                  if (pass == false)
                     printf("No such client mon!\n");
                  else{
                     kick(client_list[j].fd);
                     close(client_list[j].fd);
                     FD_CLR(client_list[j].fd, &read_fds);
                     client_list.erase(client_list.begin() + j);

                  }
               }

               if (strcmp(buffer, "quit") == 0){
                  for (j = 0; j < client_list.size(); j++){
                     kick(client_list[j].fd);
                     close(client_list[j].fd);
                     FD_CLR(client_list[j].fd, &read_fds);
                  }

                  client_list.clear();
                  close(sockfd);
                  return 0;
               }
            }

            else {
               // Receiving data from clients
               memset(auxiliary, 0, SENDLEN);
               if ((n = recv(i, auxiliary, sizeof(auxiliary), 0)) <= 0) {
                  if (n != 0) {
                     fprintf(stderr, "ERROR in recv\n");
                     return -1;
                  }
                  // Eliminating dead client from client list
                  for (j = 0; j < client_list.size(); j++){
                     if (i == client_list[j].fd){
                        printf("Client %s hung up\n", client_list[j].name);
                        client_list.erase(client_list.begin() + j);
                        break;
                     }
                  }
                  close(i);
                  FD_CLR(i, &read_fds); 
               }

               else { // Someone is talking to me!
                  t = *((msg *) auxiliary);

                  switch(t.type){
                     case 23: { // listclients
                        memset(t.payload, 0, SENDLEN);
                        for (j = 0; j < client_list.size(); j++){
                           if (i != client_list[j].fd){
                              strcat(t.payload, client_list[j].name);
                              strcat(t.payload, "\n");
                           }
                        }

                        // Sending message
                        n = send(i, &t, strlen(t.payload) + sizeof(int), 0);
                        if (n < 0){
                           fprintf(stderr, "ERROR writing to socket\n");
                           return -1;
                        }

                        break;
                     }
                     case 24: { // infoclient <client_name>
                        pass = false;
                        for (j = 0; j < client_list.size(); j++){
                           if (strcmp(t.payload, client_list[j].name) == 0){
                              // Find client
                              pass = true;
                              dummy_client = client_list[j];
                              break;
                           }
                        }
                        if (pass == false){
                           memset(t.payload, 0, SENDLEN);
                           t.type = 113;
                        }
                        else {
                           memset(t.payload, 0, SENDLEN);
                           memcpy(t.payload, &dummy_client, sizeof(client_type));
                        }
                        // Sending info about that client
                        n = send(i, &t, sizeof(msg), 0);
                        if (n < 0){
                           fprintf(stderr, "ERROR writing to socket\n");
                           return -1;
                        }
                        break;
                     }
                     case 25: { // message - requesting info about a client
                        for (j = 0; j < client_list.size(); j++){
                           if (strcmp(t.payload, client_list[j].name) == 0){
                              // Find client
                              dummy_client = client_list[j];
                              break;
                           }
                        }
                        memset(t.payload, 0, SENDLEN);
                        memcpy(t.payload, &dummy_client, sizeof(client_type));
                        // Sending info about that client
                        //printf("Requested info about %s\n", dummy_client.name);
                        n = send(i, &t, sizeof(msg), 0);
                        if (n < 0){
                           fprintf(stderr, "ERROR writing to socket\n");
                           return -1;
                        }
                        break;
                     }
                     default: {
                        printf("Invalid command!\n");
                        break;
                     }
                  }
               }

            }

         }
      }
   }

   close(sockfd);

   return 0;
}


