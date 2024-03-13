CC = g++
SOURCE = remoteClient.cpp
OBJS = remoteClient.o
EXEC = remoteClient
FLAGS = -Wall -Werror

$(EXEC): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(EXEC)

run: $(EXEC)
	./$(EXEC)

default: $(EXEC)
	./$(EXEC) -i 127.0.0.1 -p 12500 -d server

valgrind:
	valgrind ./$(EXEC)

clean:
	rm -f $(EXEC) $(OBJS)
	clear
