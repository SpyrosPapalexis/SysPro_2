CC = g++
SOURCE = dataServer.cpp queue.cpp
OBJS = dataServer.o queue.o
EXEC = dataServer
FLAGS = -Wall -Werror -pthread

$(EXEC): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o $(EXEC)

run: $(EXEC)
	./$(EXEC)

default: $(EXEC)
	./$(EXEC) -p 12500 -s 2 -q 2 -b 512

valgrind:
	valgrind ./$(EXEC)

clean:
	rm -f $(EXEC) $(OBJS)
	clear
