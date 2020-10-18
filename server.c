#include "funcs.h"

int server_udp_data = 0;

pthread_mutex_t mutex;
long int estimate_RTT = TIMEOUT;
int sendList[WINDOW_LENGTH];
struct timeval timeList[WINDOW_LENGTH];
int jeton = WINDOW_LENGTH;
int sequence_repeat = 0;


bool send_done = false;

struct sockaddr_in client;
struct timeval timeoutvalue;
socklen_t client_size = sizeof(struct sockaddr);



void *listen_ack()
{
  printf("Starting listening thread\n");
  struct timeval receive_time;

  char ack_buffer[9];
  char ack_number[6];

  int ancient_sequence_number_received = 0;
  int sequence_number_received = 0;
  
  int spot;

  int repeat_number = 0;  

  while (!send_done)
  {
    int ack_msglen = recvfrom(server_udp_data, ack_buffer, 9, 0, (struct sockaddr *)&client, &client_size);

    printf("Client-%d(%d)>>%s\n", client.sin_port, ack_msglen, ack_buffer);

    printf("ACK reçu %s\n", ack_buffer);

    if (ack_msglen < 0)
    {
      printf("Error receiving Message\n");
      return NULL;
    }

    if (strncmp(ack_buffer, "ACK", 3) != 0)
    {
      printf("Bad ACK format\n");
    }

    memset(ack_number, 0, 6);
    strncpy(ack_number, ack_buffer + 3, 6);
    sequence_number_received = atoi(ack_number);

    if(sequence_number_received > ancient_sequence_number_received)
    {
      pthread_mutex_lock(&mutex);
      //printf("MUTEX lock après reception de l'ACK\n");
      spot = find(sendList, sequence_number_received);
      if (spot == -1)
      {
        printf("ACK number greater than ancient but not in the list : %d\n", sequence_number_received);
        printIntList(sendList);
        pthread_mutex_unlock(&mutex);
        //printf("MUTEX UNlock après reception de l'ACK\n");

      }
      else
      {
        printf("Good ACK number received : %d following ancient %d\n", sequence_number_received, ancient_sequence_number_received);
        receive_time = getTime();
        estimate_RTT = estimateRTT(timeList[spot], receive_time, estimate_RTT);
        timeoutvalue.tv_usec = estimate_RTT;
        printf("Estimate RTT %ld \n", (timeoutvalue.tv_usec));
        sendList[spot] = 0;
        timeList[spot] = getTime();
        //remove all sequence number between the ancient and the new one
        jeton ++;
        for (int i = ancient_sequence_number_received; i < sequence_number_received; i++)
        {
          spot = find(sendList, i);
          if(spot != -1)
          {
            sendList[spot] = 0;
            timeList[spot] = getTime();
            jeton ++;
          }
        }
        pthread_mutex_unlock(&mutex);
        //printf("MUTEX UNlock après reception de l'ACK\n");

        ancient_sequence_number_received = sequence_number_received;
        repeat_number = 0;
      }
      
    }
    else if 
    (sequence_number_received == ancient_sequence_number_received)
    {
      repeat_number ++;
      if(repeat_number == 3)
      {
        pthread_mutex_lock(&mutex);
        //printf("MUTEX lock pour informer d'une répétition de ACK\n");
        sequence_repeat = sequence_number_received;
        pthread_mutex_unlock(&mutex);
        //printf("MUTEX UNlock pour informer d'une répétition de ACK\n");
      }
    }
    else
    {
      printf("ACK number less than ancient : DISREGARD\n");
    }
  }
  return NULL;
}

int main(int argc, char *argv[])
{
  printf("Henlo\n");

  struct sockaddr_in server_address, client, server_address_data;
  int optval = 1; // To set SO_REUSEADDR to 1
  int port = 0;
  int data_port = 0;
  int seq_number = 0;
  int token_buffer = 0;
  int sequence_repeat_buffer = 0;
  long int estimate_RTT_buffer = 0;

  size_t read;
  pid_t fork_pid = 0;
  FILE *file;
  pthread_t threadack;

  char buffer[FILE_BUFFER_SIZE];
  unsigned char file_buffer[FILE_BUFFER_SIZE];
  int msglen = 0;
  //int bytes_send = 0;
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
    /*
    int portnumber = tcp_over_udp_accept(server_udp, data_port, &client);

    printf("retour de l'acceptation de connection : %d\n", portnumber);
    */
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
    server_udp_data = socket(AF_INET, SOCK_DGRAM, 0);
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
      close(server_udp_data);
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

      //server_address.sin_port = htons(data_port);
      //server_udp = socket(AF_INET, SOCK_DGRAM, 0);
      //setsockopt(server_udp, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

      /*if (bind(server_udp, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
      {
        close(server_udp);
        error("UDP Data connection bind failed\n");
      }
      printf("New socket created on port %d\n", data_port);
      */
      if (pthread_mutex_init(&mutex, NULL) != 0)
      {
        error("Mutex initialization failed");
      }

      //Creating the listening thread
      pthread_create(&threadack, NULL, listen_ack, NULL);

      // Starting here: we are connected to a client
      memset(&buffer, 0, FILE_BUFFER_SIZE);
      memset(&file_buffer, 0, FILE_BUFFER_SIZE);
      printf("Waiting for new message\n");

      if (recvfrom(server_udp_data, buffer, FILE_BUFFER_SIZE, 0, (struct sockaddr *)&client, &client_size) < 0)
      {
        perror("Unable to receive message from client\n");
      }

      printf("Client-%d(%d)>>%s\n", data_port, msglen, buffer);

      file = fopen(buffer, "rb");
      if (file == NULL)
      {
        error("Problème lors de l'ouverture du fichier");
      }

      memset(&sendList, 0, WINDOW_LENGTH * sizeof(int));
      for (int j = 0; j < WINDOW_LENGTH; j ++)
      {
        timeList[j] = getTime();
      }

      fseek(file, 0L, SEEK_END);
      int fileSize = ftell(file);
      printf("taille du fichier : %d\n", fileSize);
      fseek(file, 0L, SEEK_SET);

      while (!send_done)
      {

        pthread_mutex_lock(&mutex);
        //printf("MUTEX lock pour récupérer les infos du jeton\n");
        token_buffer = jeton;
        sequence_repeat_buffer = sequence_repeat;
        pthread_mutex_unlock(&mutex);
        //printf("MUTEX UNlock pour récupérer les infos du jeton\n");

        if (token_buffer > 0)
        {

          if (!end_file)
          {
            seq_number++;

            //put sequence number into buffer
            memset(buffer, 0, FILE_BUFFER_SIZE);
            printf("seq_number: %06d\n", seq_number);
            sprintf(buffer, "%06d\n", seq_number);
            for (int k=0; k<6; k ++)
            {
              file_buffer[k] = (unsigned char) buffer[k];
            }

            //put text into buffer
            fseek(file, (seq_number - 1) * (FILE_BUFFER_SIZE - 6), SEEK_SET);

            read = fread(&file_buffer[6],1,sizeof(file_buffer)-6,file);
            printf("%d lu\n",(int) read);
        
            //put sequence number and send time into lists into the first available spot
            pthread_mutex_lock(&mutex);
            //printf("MUTEX lock avant envoi de packet\n");

            spot = find(sendList, 0);
            sendList[spot] = seq_number;
            timeList[spot] = getTime();

            printIntList(sendList);

            jeton--;

            pthread_mutex_unlock(&mutex);
            //printf("MUTEX UNlock avant envoi de packet\n");

            if((int)read < FILE_BUFFER_SIZE - 6)
            {
              printf("%d (bytes lus) < %d : fin du fichier détécté ", (int)read, FILE_BUFFER_SIZE - 6);
              sendto(server_udp_data, file_buffer, (int) read, 0, (struct sockaddr *)&client, client_size);
              end_file = true;
            }
            else
            {
              sendto(server_udp_data, file_buffer, sizeof(file_buffer), 0, (struct sockaddr *)&client, client_size);
            }           
          }
          else
          {
            printf("Fin du fichier reçu : attente du numéro de séquence %06d\n", seq_number);
            pthread_mutex_lock(&mutex);
            //printf("MUTEX lock pour savoir si les derniers packets du fichier ont été evoyés\n");
            spot = find(sendList, seq_number);
            pthread_mutex_unlock(&mutex);
            //printf("MUTEX UNlock pour savoir si les derniers packets du fichier ont été evoyés\n");
            if (spot == -1)
            {
              pthread_mutex_lock(&mutex);
              send_done = true;
              pthread_mutex_unlock(&mutex);
            }
            else
            {
              
              estimate_RTT_buffer = estimate_RTT;
              //printf("WAIT for %ld microseconds\n", estimate_RTT_buffer);
              usleep(estimate_RTT_buffer);
              //printf("FIN WAIT\n");
            }
            
          }
        }
        else if (token_buffer == 0)
        {
          estimate_RTT_buffer = estimate_RTT;
          //printf("WAIT for %ld microseconds\n", estimate_RTT_buffer);
          usleep(estimate_RTT_buffer);
          //printf("FIN WAIT\n");
        }

        if(sequence_repeat_buffer != 0)
        {          
          printf("Renvoie de 3 packets suivant la séquence: %06d\n", sequence_repeat_buffer);
          for (int i = 1; i < 4; i++)
          {
            memset(buffer, 0, FILE_BUFFER_SIZE);
            printf("seq_number: %06d\n", sequence_repeat_buffer + i);
            sprintf(buffer, "%06d\n", sequence_repeat_buffer + i);
            for (int k=0; k<6; k ++)
            {
              file_buffer[k] = (unsigned char) buffer[k];
            }
            //put text into buffer
            fseek(file,(sequence_repeat_buffer + i - 1) * (FILE_BUFFER_SIZE - 6), SEEK_SET);

            read = fread(&file_buffer[6],1,sizeof(file_buffer)-6,file);
            printf("%d lu\n",(int) read);
            
            if((int)read < FILE_BUFFER_SIZE - 6)
            {
              printf("%d (bytes lus) < %d : fin du fichier détécté ", (int)read, FILE_BUFFER_SIZE - 6);
              sendto(server_udp_data, file_buffer, (int) read, 0, (struct sockaddr *)&client, client_size);
              end_file = true;
            }
            else
            {
              sendto(server_udp_data, file_buffer, sizeof(file_buffer), 0, (struct sockaddr *)&client, client_size);
            }   
          }
          pthread_mutex_lock(&mutex);
          //printf("MUTEX lock pour remettre le repeat sequence à zéro\n");
          sequence_repeat = 0;
          pthread_mutex_unlock(&mutex);
          //printf("MUTEX UNlock pour remettre le repeat sequence à zéro\n");
        }
        /*int random = rand();
        if (random > 0.9)
        {
          //look to resend packets
          pthread_mutex_lock(&mutex);
          for (int i = 0; i < WINDOW_LENGTH; i++)
          {
            if (sendList[i] != 0)
            {
              if (timeout(timeList[i], estimate_RTT))
              {
                printf("Resending seq_number: %06d\n", sendList[i]);

                //put sequence number into buffer
                memset(buffer, 0, FILE_BUFFER_SIZE);
                printf("this is your seq_number: %06d\n", sendList[i]);
                sprintf(buffer, "%06d\n", sendList[i]);

                //put text into buffer
                fseek(file, 0L, (sendList[i] - 1) * (FILE_BUFFER_SIZE - 6));
                printf("Placement dans le fichier: %d\n", (sendList[i] - 1) * (FILE_BUFFER_SIZE - 6));
                putFileIntoBuffer(file, buffer, FILE_BUFFER_SIZE);

                sendto(server_udp_data, buffer, strlen(buffer), 0, (struct sockaddr *)&client, client_size);

                //recalcul du RTT
                estimate_RTT = estimateRTT(timeList[i], getTime(), estimate_RTT);
                timeoutvalue.tv_usec = estimate_RTT;
              }
            }
           }
        
        }

          pthread_mutex_unlock(&mutex);*/
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
