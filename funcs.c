#include "funcs.h"
#include <stdbool.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int tcp_over_udp_connect(int fd, struct sockaddr_in *server)
{
    char msg[32];
    int n;
    socklen_t server_len = sizeof(struct sockaddr);
    printf("Sending SYN\n");

    n = sendto(fd, "SYN\n", 4, 0, (struct sockaddr *)server, server_len);
    if (n < 0)
    {
        perror("Error sending SYN packet\n");
        return -1;
    }

    printf("SYN sent\n");

    memset(msg, 0, 32);
    n = recvfrom(fd, &msg, 32, 0, (struct sockaddr *)server, &server_len);

    if (n < 0)
    {
        perror("Error receiving SYN-ACK\n");
        return -1;
    }

    if (strncmp(msg, "SYN-ACK", 7) != 0)
    {
        perror("SYN-ACK not received\n");
        return -1;
    }

    printf("SYN-ACK received\n");

    char data_port[6];
    memset(data_port, 0, 6);

    strncpy(data_port, msg + 8, 6);

    printf("Sending ACK\n");
    n = sendto(fd, "ACK\n", 4, 0, (struct sockaddr *)server, server_len);
    if (n < 0)
    {
        perror("Error sending ACK packet\n");
        return -1;
    }

    printf("ACK sent\n");
    return atoi(data_port);
}

int tcp_over_udp_accept(int fd, int data_port, struct sockaddr_in *client)
{
    printf("Waiting for connection\n");
    char buffer[32];
    int n;
    socklen_t client_size = sizeof(struct sockaddr);

    char SYN_ACK[16];
    sprintf(SYN_ACK, "SYN-ACK%d\n", data_port);

    memset(buffer, 0, 32);
    n = recvfrom(fd, &buffer, 32, 0, (struct sockaddr *)client, &client_size);

    if (strcmp(buffer, "SYN") != 0)
    {
        perror("Connection must start with SYN\n");
        return -1;
    }
    printf("SYN received\n");

    n = sendto(fd, SYN_ACK, strlen(SYN_ACK), 0, (struct sockaddr *)client, client_size);
    if (n < 0)
    {
        perror("Unable to send SYN-ACK\n");
        return -1;
    }
    printf("SYN-ACK sent\n");

    memset(buffer, 0, 32);

    n = recvfrom(fd, &buffer, 32, 0, (struct sockaddr *)client, &client_size);
    if (strcmp(buffer, "ACK") != 0)
    {
        perror("ACK not received\n");
        return -1;
    }
    printf("ACK received. Connected\n");

    return data_port;
}

int safe_send(int fd, char *buffer, struct sockaddr_in *client, int seq_number)
{
    socklen_t client_size = sizeof(struct sockaddr);
    char ack_buffer[9];
    int ack_msglen;
    char ack_number[6];
    bool flag = true;
    int msglen = 0;
    int sequence_number_received = 0;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;
    fd_set receive_fd_set;


    while(flag)
    {
      msglen = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *)client, client_size);

      if (msglen < 0)
      {
          perror("Error sending message\n");
          return -1;
      }
      printf("%d bytes sent.\n", msglen);

      FD_ZERO (&receive_fd_set);
      FD_SET (fd, &receive_fd_set);

      int rcv = select(FD_SETSIZE,&receive_fd_set, NULL, NULL, &timeout);
      if (rcv < 0)
      {
        error("Error during select");
      }
      else if (rcv == 0)
      {
        printf("Timeout, Resending\n");
        timeout.tv_usec = TIMEOUT;
      }
      else
      {
        printf("Time to receive %ld \n", (TIMEOUT-timeout.tv_usec));
        timeout.tv_usec = TIMEOUT;
        ack_msglen = recvfrom(fd, ack_buffer, 9, 0, (struct sockaddr *)client, &client_size);

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


        if(sequence_number_received == seq_number)
        {
          printf("Good ACK number received : %d\n", sequence_number_received);
          flag = false;

        }else{
          printf("Ack number received different from expecting : %d\n", sequence_number_received);
          printf("Resending");
        }
      }

    }
    return msglen;
}

int safe_recv(int fd, char *buffer, struct sockaddr_in *client, int seq_number)
{
    socklen_t client_size = sizeof(struct sockaddr);

    int msglen = recvfrom(fd, buffer, MAX_DGRAM_SIZE, 0, (struct sockaddr *)client, &client_size);

    if (msglen < 0)
    {
        perror("Error receiving message\n");
        return -1;
    }
    char ack[12];
    memset(ack, 0, 12);

    int new_seq_number = seq_number + msglen;

    sprintf(ack, "ACK-%06d\n", new_seq_number);

    /*int ack_msglen = sendto(fd, ack, strlen(ack), 0, (struct sockaddr *)client, client_size);

    if (ack_msglen < 0)
    {
        error("Error sending ACK\n");
    }*/

    return msglen;
}

int receiveFile(int descripteur, char *buffer, struct sockaddr_in *client_addr, int seq_number)
{
    int endFile = 0;
    int msglen = 0;
    int new_seq_number = seq_number;
    int i;

    while (endFile == 0)
    {
        msglen = safe_recv(descripteur, buffer, client_addr, new_seq_number);
        new_seq_number += msglen;
        for (i = 0; i < msglen; i++)
        {
            if (buffer[i] == EOF)
            {
                int fileSize = seq_number - new_seq_number;
                endFile = 1;
                return fileSize;
            }
            else
                printf("%c", buffer[i]);
        }
    }
    return -1;
}

int sendFile(int descripteur, struct sockaddr_in *client_addr, FILE *file)
{

    char buffer[FILE_BUFFER_SIZE];
    int msglen = 0;
    int bytes_send = 0;
    bool flag = false;
    int seq_number = 0;

    fseek(file, 0L, SEEK_END);
    int fileSize = ftell(file);
    printf("taille du fichier : %d\n", fileSize);
    fseek(file, 0L, SEEK_SET);

    while (!flag)
    {
        seq_number ++;
        printf("this is your seq_number: %d\n", seq_number);
        memset(buffer, 0, FILE_BUFFER_SIZE);
        sprintf(buffer, "%06d\n", seq_number);
        flag = putFileIntoBuffer(file, buffer, FILE_BUFFER_SIZE);
        msglen = safe_send(descripteur, buffer, client_addr, seq_number);
        bytes_send += msglen;
        printf("total bytes send: %d\n", bytes_send);
        fseek(file, 0L, bytes_send - 6*seq_number);
    }

    printf("This is you seq_number at the end : %d\n", seq_number);
    return bytes_send;
}

bool putFileIntoBuffer(FILE *file, char *buffer, int bufferSize)
{
    int i;
    if (file == NULL)
    {
        strcpy(buffer, "No File Found");
        buffer[strlen("No File Found")] = EOF;
        return false;
    }
    for (i = 6; i < bufferSize; i++)
    {
        buffer[i] = fgetc(file);
        if (buffer[i] == EOF)
            return true;
    }
    return false;
}
