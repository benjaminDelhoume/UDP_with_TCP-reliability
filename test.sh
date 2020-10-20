#!/bin/bash

echo "debut du test"

server_address='134.214.202.243'
#ANCIENT_FILE_BUFFER_SIZE='1500'
ANCIENT_WINDOW_LENGTH='128'
ANCIENT_ALPHA='0.99'
ANCIENT_TIMEOUT='50000'

echo "$server_address"

for WINDOW_LENGTH in '64' '96' '128';
do
  for FILE_BUFFER_SIZE in '1500'
    do
      for ALPHA in '0.1' '0.2' '0.4' '0.5' '0.7' '0.9' '0.99'
        do
          for TIMEOUT in '1000' '5000' '10000' '15000' '50000'
            do
              echo "WINDOW_LENGTH : $WINDOW_LENGTH"
              echo "FILE_BUFFER_SIZE : $FILE_BUFFER_SIZE"
              echo "ALPHA : $ALPHA"
              echo "TIMEOUT : $TIMEOUT"

              sed -i -e "s/#define WINDOW_LENGTH $ANCIENT_WINDOW_LENGTH/#define WINDOW_LENGTH $WINDOW_LENGTH/g" funcs.h
              #sed -i -e "s/#define FILE_BUFFER_SIZE $ANCIENT_FILE_BUFFER_SIZE/#define FILE_BUFFER_SIZE $FILE_BUFFER_SIZE/g" funcs.h
              sed -i -e "s/#define ALPHA $ANCIENT_ALPHA/#define ALPHA $ALPHA/g" funcs.h
              sed -i -e "s/#define TIMEOUT $ANCIENT_TIMEOUT/#define TIMEOUT $TIMEOUT/g" funcs.h
              printf "%b\n" >> server.c

              ANCIENT_WINDOW_LENGTH="$WINDOW_LENGTH"
              #ANCIENT_FILE_BUFFER_SIZE="$FILE_BUFFER_SIZE"
              ANCIENT_ALPHA="$ALPHA"
              ANCIENT_TIMEOUT="$TIMEOUT"

              make server
              ./server 1234 > server.log 2>&1 &
              sleep 1
              time ./client1 $server_address 1234 1M.bin 0

              killall server
              rm server.log
              sleep 1
              echo
              echo
              echo
            done
        done
    done
done
