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

      // Starting here: we are connected to a client
      while (1)
      {
        memset(&buffer, 0, MAX_DGRAM_SIZE);
        printf("Waiting for new message\n");
        recvsize = safe_recv(server_udp_data, buffer, &client, receive_sequence_number);

        if (strcmp(buffer, "FIN\n") == 0)
        {
          printf("FIN received from client. Closing connection\n");
          break;
        }

        if (recvsize < 0)
        {
          perror("Unable to receive message from client\n");
        }

        receive_sequence_number += recvsize;
        printf("Client-%d(%d)>>%s\n", data_port, recvsize, buffer);

        file = fopen(buffer, "r");
        if (file == NULL)
        {
          error("Problème lors de l'ouverture du fichier");
        }
        recvsize = sendFile(server_udp_data, &client, file);
        send_sequence_number += recvsize;
        sendto(server_udp_data, "FIN", strlen("FIN"), 0, (struct sockaddr *)&client, sizeof(client));

        printf("Fin d'envoi du fichier \n");
        fclose(file);
      }
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
