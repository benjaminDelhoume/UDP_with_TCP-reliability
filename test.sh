#!/bin/bash

echo "DEBUT du test"

server_address='134.214.202.243'
#ANCIENT_FILE_BUFFER_SIZE='4096'
ANCIENT_ALPHA='0.99'
ANCIENT_TIMEOUT='10000'

echo "$server_address"

for FILE_BUFFER_SIZE in '1500'
  do
    for ALPHA in '0.1' '0.2' '0.3' '0.4' '0.5' '0.6' '0.7' '0.8' '0.9' '0.99'
      do
        for TIMEOUT in '500' '1000' '5000' '10000' '15000'
          do
            echo "client $i"
            echo "FILE_BUFFER_SIZE : $FILE_BUFFER_SIZE"
            echo "ALPHA : $ALPHA"
            echo "TIMEOUT : $TIMEOUT"

            #sed -i -e "s/#define FILE_BUFFER_SIZE $ANCIENT_FILE_BUFFER_SIZE/#define FILE_BUFFER_SIZE $FILE_BUFFER_SIZE/g" funcs.h
            sed -i -e "s/#define ALPHA $ANCIENT_ALPHA/#define ALPHA $ALPHA/g" funcs.h
            sed -i -e "s/#define TIMEOUT $ANCIENT_TIMEOUT/#define TIMEOUT $TIMEOUT/g" funcs.h

            #ANCIENT_FILE_BUFFER_SIZE="$FILE_BUFFER_SIZE"
            ANCIENT_ALPHA="$ALPHA"
            ANCIENT_TIMEOUT="$TIMEOUT"

            make server
            ./server 1234 2000 > server.log 2>&1 &
            sleep 1
            time ./client2 $server_address 1234 1M.bin 0
            sleep 1

            killall server
            rm server.log

            echo
            echo
            echo
        done
    done
 done
 echo "FIN test"
