#include "funcs2.h"

void error(char *msg)
{
  perror(msg);
  exit(1);
}

int tcp_over_udp_accept(int fd, int data_port, struct sockaddr_in *client)
{
  //printf("Waiting for connection\n");
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
  //printf("SYN received\n");

  n = sendto(fd, SYN_ACK, strlen(SYN_ACK), 0, (struct sockaddr *)client, client_size);
  if (n < 0)
  {
    perror("Unable to send SYN-ACK\n");
    return -1;
  }
  //printf("SYN-ACK sent\n");

  memset(buffer, 0, 32);

  n = recvfrom(fd, &buffer, 32, 0, (struct sockaddr *)client, &client_size);
  if (strcmp(buffer, "ACK") != 0)
  {
    perror("ACK not received\n");
    return -1;
  }
  //printf("ACK received. Connected\n");

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
    msglen = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *)client, client_size);

    send_time = getTime();

    if (msglen < 0)
    {
      perror("Error sending message\n");
      return -1;
    }
    //printf("%d bytes sent.\n", msglen);

    FD_ZERO(&receive_fd_set);
    FD_SET(fd, &receive_fd_set);

    int rcv = select(FD_SETSIZE, &receive_fd_set, NULL, NULL, &timeout);
    if (rcv < 0)
    {
      error("Error during select");
    }
    else if (rcv == 0)
    {
      //printf("Timeout, Resending\n");
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

      //printf("Estimate RTT %ld \n", (timeout.tv_usec));

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
        //printf("Good ACK number received : %d\n", sequence_number_received);
        flag = false;
      }
      else
      {
        //printf("Ack number received different from expecting : %d\n", sequence_number_received);
        //printf("Resending");
      }
    }
  }
  return msglen;
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
  }
  else
  {
    //proceder à la retenu
    new_estimate_RTT = ALPHA * estimate_RTT + (1 - ALPHA) * (1000000 + receive_time.tv_usec - send_time.tv_usec);
  }

  return (long int)new_estimate_RTT;
}

int find(int *list, int sequence_number)
{
  for (int i = 0; i < WINDOW_LENGTH; i++)
  {
    if (list[i] == sequence_number)
    {
      return i;
    }
  }
  return -1;
}

int findMin(int *list)
{
  int min = 0;
  for (int i = 1; i < WINDOW_LENGTH; i++)
  {
    if ((list[i] < list[min] && list[i] != 0) || list[min] == 0)
    {
      min = i;
    }
  }
  return min;
}

bool timeout(struct timeval send_time, long int estimate_RTT)
{
  struct timeval now = getTime();
  if (now.tv_usec >= estimate_RTT)
  {
    now.tv_usec -= estimate_RTT;
  }
  else
  {
    //procéder à la retenu
    now.tv_sec = -1;
    now.tv_usec = 1000000 + now.tv_usec - estimate_RTT;
  }
  return (timercmp(&now, &send_time, >=));
}

void printIntList(int *list)
{
  //printf("SendList : ");
  for (int i = 0; i < WINDOW_LENGTH; i++)
  {
    //printf("%d, ",list[i]);
  }
  //printf("\n");
}

void printTimeList(struct timeval *list)
{
  //printf("SendList : ");
  for (int i = 0; i < WINDOW_LENGTH; i++)
  {
    //printf("%ld, ",list[i].tv_usec);
  }
  //printf("\n");
}
