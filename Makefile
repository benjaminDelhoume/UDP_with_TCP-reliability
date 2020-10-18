CFLAGS = -g -Wall -DDEBUG 
EXEC = server
SRC= $(EXEC:=.c)
OBJ= $(SRC:.c=.o)

.PRECIOUS: $(OBJ)

all: $(EXEC)

server: server.o funcs.o funcs.h

%: %.o
	gcc  $^ -o $@ -lpthread

%.o: %.c %.h
	gcc $(CFLAGS) -c $< -o $@ -lpthread

clean: 
	\rm -rf $(OBJ) *.o $(EXEC)
