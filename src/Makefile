CFLAGS = -g -Wall -DDEBUG
EXEC = server1-GreDel
SRC= $(EXEC:=.c)
OBJ= $(SRC:.c=.o)

.PRECIOUS: $(OBJ)

all: $(EXEC)

server1-GreDel: server1-GreDel.o funcs.o funcs.h
server2-GreDel: server2-GreDel.o funcs.o funcs.h

%: %.o
	gcc  $^ -o $@ -lpthread

%.o: %.c %.h
	gcc $(CFLAGS) -c $< -o $@ -lpthread

clean:
	\rm -rf $(OBJ) *.o $(EXEC)

move:
	mv server1-GreDel bin/
