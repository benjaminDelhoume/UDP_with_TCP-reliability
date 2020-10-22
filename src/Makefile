CFLAGS = -g -Wall -DDEBUG
EXEC = server1-GreDel server2-GreDel server3-GreDel move
SRC= $(EXEC:=.c)
OBJ= $(SRC:.c=.o)

.PRECIOUS: $(OBJ)

all: $(EXEC)

server1-GreDel: server1-GreDel.o funcs.o funcs.h
server2-GreDel: server2-GreDel.o funcs2.o funcs2.h
server3-GreDel: server3-GreDel.o funcs3.o funcs3.h

%: %.o
	gcc  $^ -o $@ -lpthread

%.o: %.c %.h
	gcc $(CFLAGS) -c $< -o $@ -lpthread

clean:
	\rm -rf $(OBJ) *.o $(EXEC)

move:
	mv server1-GreDel ../bin/
	mv server2-GreDel ../bin/
	mv server3-GreDel ../bin/
