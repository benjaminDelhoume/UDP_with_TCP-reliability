#!/bin/bash

echo "debut du test"

server_address='134.214.202.243'
ANCIENT_FILE_BUFFER_SIZE='4096'

echo "$server_address"
echo "$ANCIENT_FILE_BUFFER_SIZE"

for i in '1' '2';
do
  echo "client $i"
  for FILE_BUFFER_SIZE in '512' '1024' '2048' '4096';
    do
      echo "FILE_BUFFER_SIZE : $FILE_BUFFER_SIZE"
      sed -i -e "s/#define FILE_BUFFER_SIZE $ANCIENT_FILE_BUFFER_SIZE/#define FILE_BUFFER_SIZE $FILE_BUFFER_SIZE/g" funcs.h
      ANCIENT_FILE_BUFFER_SIZE="$FILE_BUFFER_SIZE"
      make server
      ./server 1234 2000 > server.log 2>&1 &
      time ./client$i $server_address 1234 moyen.txt 0
    done
    killall server
done
