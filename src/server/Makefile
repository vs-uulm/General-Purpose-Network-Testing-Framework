OBJS = main.o
DOBJS = debug.o

CPP = main.cpp
HPP = input_parser.hpp server_task_handler.hpp L4.hpp L7.hpp L7_web_browsing.hpp L7_file_sharing.hpp L7_F1460B.hpp L7_server_storage_marketplace.hpp  ../misc/Distribution_Functions.hpp ../misc/useful_functions.hpp

CC = g++
DEBUG = -DDEBUG
CFLAGS = -c
PREFLAGS = -Wall -std=gnu++17
POSTFLAGS = -pthread

help:
	@echo Make Commands: main, debug, clean

main: $(OBJS)
	$(CC) $(PREFLAGS) $(OBJS) -o main $(POSTFLAGS)

debug: $(DOBJS)
	$(CC) $(PREFLAGS) $(DEBUG) $(DOBJS) -o debug $(POSTFLAGS)

$(DOBJS): $(CPP) $(HPP)
	$(CC) $(PREFLAGS) $(DEBUG) $(CFLAGS) main.cpp -o debug.o $(POSTFLAGS)

$(OBJS): $(CPP) $(HPP)
	$(CC) $(PREFLAGS) $(CFLAGS) main.cpp $(POSTFLAGS)

clean:
	rm -f *.o main debug
