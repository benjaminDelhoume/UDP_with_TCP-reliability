#include "funcs.h"
#include <stdbool.h>

void error(char *msg)
{
  perror(msg);
  exit(1);
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

int safe_send(int fd,unsigned char *buffer, int buffer_length, struct sockaddr_in *client, int seq_number)
{
  socklen_t client_size = sizeof(struct sockaddr);
  char ack_buffer[9];
  int ack_msglen;
  char ack_number[6];
  bool flag = true;
  int msglen = 0;
  int sequence_number_received = 0;
  long int estimate_RTT;
  struct timeval timeout;
  struct timeval send_time;
  struct timeval receive_time;
  fd_set receive_fd_set;

  //initialisation du RTT
  timeout.tv_sec = 0;
  timeout.tv_usec = TIMEOUT;
  estimate_RTT = TIMEOUT;
  while (flag)
  {
    msglen = sendto(fd, buffer, buffer_length, 0, (struct sockaddr *)client, client_size);

    send_time = getTime();

    if (msglen < 0)
    {
      perror("Error sending message\n");
      return -1;
    }
    printf("%d bytes sent.\n", msglen);

    FD_ZERO(&receive_fd_set);
    FD_SET(fd, &receive_fd_set);

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
      ack_msglen = recvfrom(fd, ack_buffer, 9, 0, (struct sockaddr *)client, &client_size);

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
  return msglen;
}

int sendFile(int descripteur, struct sockaddr_in *client_addr, FILE *file)
{

  unsigned char file_buffer[FILE_BUFFER_SIZE];
  char buffer[FILE_BUFFER_SIZE];
  bool flag = false;
  int seq_number = 0;
  size_t read;

  fseek(file, 0L, SEEK_END);
  int fileSize = ftell(file);
  printf("taille du fichier : %d\n", fileSize);
  fseek(file, 0L, SEEK_SET);

  while (!flag)
  {
    seq_number++;
    printf("this is your seq_number: %d\n", seq_number);
    memset(buffer, 0, FILE_BUFFER_SIZE);
    memset(file_buffer, 0, FILE_BUFFER_SIZE);
    sprintf(buffer, "%06d\n", seq_number);
    for (int k = 0; k < 6; k++)
    {
      file_buffer[k] = (unsigned char)buffer[k];
    }

    //put text into buffer
    fseek(file, (seq_number - 1) * (FILE_BUFFER_SIZE - 6), SEEK_SET);
    read = fread(&file_buffer[6], 1, sizeof(file_buffer) - 6, file);

    if ((int)read < FILE_BUFFER_SIZE - 6)
    {
      printf("%d (bytes lus) < %d : fin du fichier détécté ", (int)read, FILE_BUFFER_SIZE - 6);
      safe_send(descripteur, file_buffer, (int) (read + 6), client_addr, seq_number);
      flag = true;
    }
    else
    {
      safe_send(descripteur, file_buffer, sizeof(file_buffer), client_addr, seq_number);
    }
  }

  printf("This is you seq_number at the end : %d\n", seq_number);
  return fileSize;
}

struct timeval getTime(void)
{
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);
  return currentTime;
}

long int estimateRTT(struct timeval send_time, struct timeval receive_time, long int estimate_RTT)
{
  long double new_estimate_RTT;
  if (receive_time.tv_usec >= send_time.tv_usec)
  {
    new_estimate_RTT = ALPHA * estimate_RTT + (1 - ALPHA) * (receive_time.tv_usec - send_time.tv_usec);
    printf(" difference normale : %ld \n", (long int)new_estimate_RTT);
  }
  else
  {
    //proceder à la retenu
    new_estimate_RTT = ALPHA * estimate_RTT + (1 - ALPHA) * (1000000 + receive_time.tv_usec - send_time.tv_usec);
    printf(" difference avec retenu : %ld \n", (long int)new_estimate_RTT);
  }

  return (long int)new_estimate_RTT;
}
