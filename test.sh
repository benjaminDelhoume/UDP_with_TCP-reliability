#!/bin/bash

echo "debut du test"

server_address = $1

for i in 'seq 1 2';
do
  echo "client $i";
  ANCIENT_FILE_BUFFER_SIZE = 1024
  for FILE_BUFFER_SIZE in 256 512 1024 2048 4096
    do
      echo "FILE_BUFFER_SIZE : $FILE_BUFFER_SIZE";
      sed -i -e "s/#define FILE_BUFFER_SIZE $ANCIENT_FILE_BUFFER_SIZE/#define FILE_BUFFER_SIZE $FILE_BUFFER_SIZE/g" funcs.h;
      ANCIENT_FILE_BUFFER_SIZE = $FILE_BUFFER_SIZE;
      make server;
      ./server 1234 2000;
      ./client$i server_address 1234 moyen.txt 0



done

