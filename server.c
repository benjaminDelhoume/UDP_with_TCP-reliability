#include "funcs.h"

pthread_mutex_t mutex;
int server_udp_data;

int seq_number = 0;
int jeton = WINDOW_LENGTH;
long int estimate_RTT;
bool send_done = false;

struct sockaddr_in client;
struct timeval timeout;
socklen_t client_size = sizeof(struct sockaddr);

int sendList[WINDOW_LENGTH];
struct timeval timeList[WINDOW_LENGTH];

void listen_ack()
{

  struct timeval send_time;
  struct timeval receive_time;

  fd_set receive_fd_set;
  FD_ZERO(&receive_fd_set);
  FD_SET(server_udp_data, &receive_fd_set);


  char ack_buffer[9];
  int ack_msglen;
  char ack_number[6];
  int sequence_number_received = 0;
  int spot;

  while (!send_done)
  {
        int ack_msglen = recvfrom(server_udp_data, ack_buffer, 9, 0, (struct sockaddr *)client, &client_size);

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

    pthread_mutex_lock(&mutex);
    spot = find(sendList, sequence_number_received);
    if ( spot != -1)
    {
      printf("Good ACK number received : %d\n", sequence_number_received);
      receive_time = getTime();
      estimate_RTT = estimateRTT(timeList[spot], receive_time, estimate_RTT);
      timeout.tv_usec = estimate_RTT;

      printf("Estimate RTT %ld \n", (timeout.tv_usec));

      sendList[spot] = 0;
      timeList[spot] = 0;

      jeton ++;
    }
    else
    {
      printf("Ack number received different from expecting : %d\n", sequence_number_received);
    }
    pthread_mutex_unlock(&mutex);
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
  pid_t fork_pid = 0;
  int send_sequence_number = 1;
  FILE *file;
  pthread_t threadack;

  char buffer[FILE_BUFFER_SIZE];
  int msglen = 0;
  int bytes_send = 0;
  bool end_file = false;
  int spot;

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
    int portnumber = tcp_over_udp_accept(server_udp, data_port, &client);

    printf("retour de l'acceptation de connection : %d\n", portnumber);

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

      server_address.sin_port = htons(portnumber);
      server_udp = socket(AF_INET, SOCK_DGRAM, 0);
      setsockopt(server_udp, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

      if (bind(server_udp, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
     {
       close(server_udp);
       error("UDP Data connection bind failed\n");
     }
     printf("New socket created on port %d\n", portnumber);

     if(pthread_mutex_init(&mutex,NULL) != 0){
       error("Mutex initialization failed");
     }
      //Creating the listening thread
      pthread_create(&threadack, NULL, listen_ack, NULL);

      // Starting here: we are connected to a client
      memset(&buffer, 0, MAX_DGRAM_SIZE);
      printf("Waiting for new message\n");

      if (recvfrom(server_udp_data, buffer, MAX_DGRAM_SIZE, 0, (struct sockaddr *)&client, &client_size) < 0)
      {
        perror("Unable to receive message from client\n");
      }

      printf("Client-%d(%d)>>%s\n", data_port, msglen, buffer);

      file = fopen(buffer, "r");
      if (file == NULL)
      {
        error("Problème lors de l'ouverture du fichier");
      }

      memset(&sendList, 0, WINDOW_LENGTH);
      memset(&timeList, 0, WINDOW_LENGTH);

      fseek(file, 0L, SEEK_END);
      int fileSize = ftell(file);
      printf("taille du fichier : %d\n", fileSize);
      fseek(file, 0L, SEEK_SET);

      while (!send_done)
      {
        if (jeton > 0)
        {


            if(!end_file)
            {
              //send packets
              pthread_mutex_lock(&mutex);
              seq_number ++;

              //put sequence number into buffer
              memset(buffer, 0, FILE_BUFFER_SIZE);
              printf("this is your seq_number: %06d\n", seq_number);
              sprintf(buffer, "%06d\n", seq_number);

              //put text into buffer
              fseek(file, 0L,(seq_number-1)*(FILE_BUFFER_SIZE-6));

              pthread_mutex_unlock(&mutex);

              end_file = putFileIntoBuffer(file, buffer, FILE_BUFFER_SIZE);

              sendto(descripteur, buffer, strlen(buffer), 0, (struct sockaddr *) client_addr, client_size);

              //put sequence number and send time into lists into the first available spot
              pthread_mutex_lock(&mutex);
              spot = find(sendList,0);
              sendList[spot] = seq_number;
              timeList[spot] = getTime();

              jeton --;
              pthread_mutex_unlock(&mutex);
            }
            else
            {
              //check if the last packet is acked
              if(find(sendList, seq_number) == -1){
                pthread_mutex_lock(&mutex);
                send_done = true;
                pthread_mutex_unlock(&mutex);
              }
            }

            //look to resend packets
            pthread_mutex_lock(&mutex);
            for(int i=0; i < WINDOW_LENGTH; i++){
              if(timeout(timeList[i], estimate_RTT)){
                printf("Resending seq_number: %06d\n", sendList[i]);

                //put sequence number into buffer
                memset(buffer, 0, FILE_BUFFER_SIZE);
                printf("this is your seq_number: %06d\n", sendList[i]);
                sprintf(buffer, "%06d\n", sendList[i]);

                //put text into buffer
                fseek(file, 0L,(sendList[i]-1)*(FILE_BUFFER_SIZE-6));
                putFileIntoBuffer(file, buffer, FILE_BUFFER_SIZE);

                sendto(descripteur, buffer, strlen(buffer), 0, (struct sockaddr *) client_addr, client_size);

                //recalcul du RTT
                estimate_RTT = estimateRTT(timeList[i], getTime(), estimate_RTT);
                timeout.tv_usec = estimate_RTT;
              }
            }

            pthread_mutex_unlock(&mutex);

          }
      }
      printf("This is you seq_number at the end : %d\n", seq_number);


      sendto(server_udp_data, "FIN", strlen("FIN"), 0, (struct sockaddr *)&client, sizeof(client));

      printf("Fin d'envoi du fichier \n");
      fclose(file);
      pthread_join(threadack, NULL);
      pthread_mutex_destroy(&mutex);
      close(server_udp_data);
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
