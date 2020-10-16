#ifndef SERVER_FUNCS_H
#define SERVER_FUNCS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h>

#define FILE_BUFFER_SIZE 1024
#define TIMEOUT 1000
#define ALPHA 0.7
#define WINDOW_LENGTH 10

void error(char *msg);

int tcp_over_udp_accept(int fd, int data_port, struct sockaddr_in *client);

int safe_send(int fd, char* buffer, struct sockaddr_in *client, int seq_number);

bool putFileIntoBuffer(FILE* file, char* buffer, int bufferSize);

struct timeval getTime(void);

long int estimateRTT(struct timeval send_time, struct timeval receive_time, long int estimate_RTT);

int find(int *list, int sequence_number);

bool timeout(struct timeval send_time, long int estimate_RTT);

void *listen_ack();

#endif
