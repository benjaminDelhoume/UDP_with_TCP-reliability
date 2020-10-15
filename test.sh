#!/bin/bash

echo "debut du test"

server_address='134.214.202.243'
ANCIENT_FILE_BUFFER_SIZE='4096'
ANCIENT_ALPHA='0.99'
ANCIENT_TIMEOUT='10000'

echo "$server_address"

for i in '1' '2';
do
  for FILE_BUFFER_SIZE in '512' '1024' '2048' '4096'
    do
      for ALPHA in '0.1' '0.2' '0.3' '0.4' '0.5' '0.6' '0.7' '0.8' '0.9' '0.99'
        do
          for TIMEOUT in '1000' '5000' '10000'
            do
              echo "client $i"
              echo "FILE_BUFFER_SIZE : $FILE_BUFFER_SIZE"
              echo "ALPHA : $ALPHA"
              echo "TIMEOUT : $TIMEOUT"

              sed -i -e "s/#define FILE_BUFFER_SIZE $ANCIENT_FILE_BUFFER_SIZE/#define FILE_BUFFER_SIZE $FILE_BUFFER_SIZE/g" funcs.h
              sed -i -e "s/#define ALPHA $ANCIENT_ALPHA/#define ALPHA $ALPHA/g" funcs.h
              sed -i -e "s/#define TIMEOUT $ANCIENT_TIMEOUT/#define TIMEOUT $TIMEOUT/g" funcs.h

              ANCIENT_FILE_BUFFER_SIZE="$FILE_BUFFER_SIZE"
              ANCIENT_ALPHA="$ALPHA"
              ANCIENT_TIMEOUT="$TIMEOUT"

              make server
              ./server 1234 2000 > server.log 2>&1 &
              sleep 2
              time ./client$i $server_address 1234 moyen.txt 0

              killall server
              killall client$i
              rm server.log
            done
        done
    done
done
