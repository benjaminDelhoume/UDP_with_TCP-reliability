#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include "funcs.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int window_len = 10;
int server_udp_data;
int seq_number = 0;
int jeton = 10;
struct sockaddr_in client;
socklen_t client_size = sizeof(struct sockaddr);
char sendList[10];

void listen_ack()
{
  struct timeval timeout;
  struct timeval send_time;
  struct timeval receive_time;
  fd_set receive_fd_set;
  FD_ZERO(&receive_fd_set);
  FD_SET(server_udp_data, &receive_fd_set);
  long int estimate_RTT;
  char ack_buffer[9];
  int ack_msglen;
  char ack_number[6];
  int sequence_number_received = 0;
  bool flag = true;

  while (true)
  {
    while (jeton != 0)
    {
      int rcv = select(FD_SETSIZE, &receive_fd_set, NULL, NULL, &timeout);
      if (rcv < 0)
      {
        error("Error during select");
      }
      else if (rcv == 0)
      {
        printf("Timeout, Resending\n");
        receive_time = getTime();
        estimate_RTT = estimateRTT(send_time, receive_time, estimate_RTT);
        timeout.tv_usec = estimate_RTT;
      }
      else
      {
        int ack_msglen = recvfrom(server_udp_data, ack_buffer, 9, 0, (struct sockaddr *)client, &client_size);

        receive_time = getTime();
        estimate_RTT = estimateRTT(send_time, receive_time, estimate_RTT);
        timeout.tv_usec = estimate_RTT;

        printf("Estimate RTT %ld \n", (timeout.tv_usec));

        if (ack_msglen < 0)
        {
          perror("Error receiving ACK\n");
          return -1;
        }

        if (strncmp(ack_buffer, "ACK", 3) != 0)
        {
          perror("Bad ACK format\n");
          return -1;
        }

        memset(ack_number, 0, 6);

        strncpy(ack_number, ack_buffer + 3, 6);

        sequence_number_received = atoi(ack_number);

        if (sequence_number_received == seq_number)
        {
          printf("Good ACK number received : %d\n", sequence_number_received);
          flag = false;
        }
        else
        {
          printf("Ack number received different from expecting : %d\n", sequence_number_received);
          printf("Resending");
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  printf("Henlo\n");
  struct sockaddr_in server_address, client, server_address_data;
  int optval = 1; // To set SO_REUSEADDR to 1
  int port = 0;
  int data_port = 0;
  int recvsize = 0;
  char buffer[MAX_DGRAM_SIZE];
  pid_t fork_pid = 0;
  int receive_sequence_number = 1; // We're waiting for the 1st byte
  int send_sequence_number = 1;
  FILE *file;
  pthread_t threadack;

  if (argc != 3)
  {
    error("USAGE: server <control_port> <data_port>\n");
  }

  port = atoi(argv[1]);
  data_port = atoi(argv[2]);

  if (port < 0 || data_port < 0)
  {
    printf("Port number must be greater than 0\n");
  }

  //create socket
  int server_udp = socket(AF_INET, SOCK_DGRAM, 0);
  setsockopt(server_udp, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

  //handle error
  if (server_udp < 0)
  {
    error("Cannot create socket\n");
  }

  printf("Binding address...\n");

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(server_udp, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
  {
    close(server_udp);
    error("UDP Bind failed\n");
  }

  while (1)
  {
    printf("Accepting connections...\n");

    // Serveur UDP
    /*
    int portnumber = tcp_over_udp_accept(server_udp, data_port, &client);

    printf("retour de l'acceptation de connection : %d\n", portnumber);
    */

    //code part creating a second socket for Ben PC
    // In order to change transform server_udp_data to server_udp
    printf("Waiting for connection\n");
    char bufferBen[32];
    int n;
    socklen_t client_size = sizeof(struct sockaddr);
    bool send_done;

    char SYN_ACK[16];
    sprintf(SYN_ACK, "SYN-ACK%d\n", data_port);

    memset(buffer, 0, 32);
    n = recvfrom(server_udp, &bufferBen, 32, 0, (struct sockaddr *)&client, &client_size);

    if (strcmp(bufferBen, "SYN") != 0)
    {
      perror("Connection must start with SYN\n");
      return -1;
    }
    printf("SYN received\n");

    n = sendto(server_udp, SYN_ACK, strlen(SYN_ACK), 0, (struct sockaddr *)&client, client_size);
    if (n < 0)
    {
      perror("Unable to send SYN-ACK\n");
      return -1;
    }
    int server_udp_data = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(server_udp_data, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    //handle error
    if (server_udp < 0)
    {
      error("Cannot create socket\n");
    }

    server_address_data.sin_family = AF_INET;
    server_address_data.sin_port = htons(data_port);
    server_address_data.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_udp_data, (struct sockaddr *)&server_address_data, sizeof(server_address)) == -1)
    {
      close(server_udp);
      error("UDP Data connection bind failed\n");
    }
    printf("New socket created on port %d\n", data_port);

    printf("SYN-ACK sent\n");

    memset(buffer, 0, 32);

    n = recvfrom(server_udp, &bufferBen, 32, 0, (struct sockaddr *)&client, &client_size);
    if (strcmp(bufferBen, "ACK") != 0)
    {
      perror("ACK not received\n");
      return -1;
    }
    printf("ACK received. Connected\n");
    data_port++;

    // Forking to serve new clients

    fork_pid = fork();

    if (fork_pid < 0)
    {
      error("Error forking process\n");
    }

    if (fork_pid == 0)
    {
      // Child process here
      close(server_udp);

      //server_address.sin_port = htons(portnumber);
      //server_udp = socket(AF_INET, SOCK_DGRAM, 0);
      //setsockopt(server_udp, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

      //Creating the listening thread
      pthread_create(&threadack, NULL, listen_ack, NULL);

      // Starting here: we are connected to a client
      while (1)
      {
        memset(&buffer, 0, MAX_DGRAM_SIZE);
        printf("Waiting for new message\n");

        int msglen = recvfrom(server_udp_data, buffer, MAX_DGRAM_SIZE, 0, (struct sockaddr *)&client, &client_size);

        if (msglen < 0)
        {
          perror("Error receiving message\n");
          return -1;
        }

        if (strcmp(buffer, "FIN\n") == 0)
        {
          printf("FIN received from client. Closing connection\n");
          break;
        }

        if (msglen < 0)
        {
          perror("Unable to receive message from client\n");
        }

        receive_sequence_number += msglen;
        printf("Client-%d(%d)>>%s\n", data_port, msglen, buffer);

        file = fopen(buffer, "r");
        if (file == NULL)
        {
          error("Problème lors de l'ouverture du fichier");
        }
        while (!send_done)
        {
          while (jeton != 0)
          {
            recvsize = sendFile(server_udp_data, &client, file);
            send_sequence_number += recvsize;
          }
        }
        sendto(server_udp_data, "FIN", strlen("FIN"), 0, (struct sockaddr *)&client, sizeof(client));

        printf("Fin d'envoi du fichier \n");
        fclose(file);
      }
      pthread_join(threadack, NULL);
      close(server_udp_data);
      exit(0);
    }
    else
    {
      printf("Child process started at PID %d\n", fork_pid);
    }
  }

  printf("Closing UDP server\n");
  close(server_udp);
  return 0;
}
