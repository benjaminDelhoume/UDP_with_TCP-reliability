#!/bin/bash

echo "debut du test"

server_address='134.214.202.243'
#ANCIENT_FILE_BUFFER_SIZE='1500'
ANCIENT_WINDOW_LENGTH='0'
ANCIENT_ALPHA='0'
ANCIENT_TIMEOUT='0'
ANCIENT_MAX_ACK_RETRANSMIT='0'

echo "$server_address"

for WINDOW_LENGTH in '4' '8' '16' '32' '64' ;
do
  for FILE_BUFFER_SIZE in '1500'
    do
      for ALPHA in '0.1' '0.3' '0.6' '0.95'
        do
          for TIMEOUT in '1000' '2500' '5000' '10000'
            do
              for MAX_ACK_RETRANSMIT in '1' '2' '3'
                do
                  echo "WINDOW_LENGTH : $WINDOW_LENGTH"
                  echo "FILE_BUFFER_SIZE : $FILE_BUFFER_SIZE"
                  echo "ALPHA : $ALPHA"
                  echo "TIMEOUT : $TIMEOUT"
                  echo "MAX_ACK_RETRANSMIT: $MAX_ACK_RETRANSMIT"

                  sed -i -e "s/#define WINDOW_LENGTH $ANCIENT_WINDOW_LENGTH/#define WINDOW_LENGTH $WINDOW_LENGTH/g" funcs.h
                  #sed -i -e "s/#define FILE_BUFFER_SIZE $ANCIENT_FILE_BUFFER_SIZE/#define FILE_BUFFER_SIZE $FILE_BUFFER_SIZE/g" funcs.h
                  sed -i -e "s/#define ALPHA $ANCIENT_ALPHA/#define ALPHA $ALPHA/g" funcs.h
                  sed -i -e "s/#define TIMEOUT $ANCIENT_TIMEOUT/#define TIMEOUT $TIMEOUT/g" funcs.h
                  sed -i -e "s/#define MAX_ACK_RETRANSMIT $ANCIENT_MAX_ACK_RETRANSMIT/#define MAX_ACK_RETRANSMIT $MAX_ACK_RETRANSMIT/g" funcs.h
                  printf "%b\n" >> server1-GreDel.c

                  ANCIENT_WINDOW_LENGTH="$WINDOW_LENGTH"
                  #ANCIENT_FILE_BUFFER_SIZE="$FILE_BUFFER_SIZE"
                  ANCIENT_ALPHA="$ALPHA"
                  ANCIENT_TIMEOUT="$TIMEOUT"
                  ANCIENT_MAX_ACK_RETRANSMIT="$MAX_ACK_RETRANSMIT"

                  make server1-GreDel
                  ./server1-GreDel 1234 > server.log 2>&1 &
                  sleep 1
                  time ../bin/client2 $server_address 1234 1M.bin 0

                  killall server1-GreDel
                  rm server.log
                  sleep 1
                  echo
                  echo
                  echo
                done
            done
        done
    done
done
