CC= gcc
THREAD=-lpthread
CFLAGS= -Wall -pedantic
SERVEROUT= ./out/server
CLIENTOUT= ./out/client
SOURCES= ./source/server.c ./source/file.c ./source/hash.c ./source/list.c ./source/Protcol.c
SOURCEC= ./source/Protcol.c ./source/client.c ./source/api.c  ./source/stringutil.c 
server: ./source/server.c
	$(CC) -I./header/*.h -o $(SERVEROUT) $(SOURCES)  $(CFLAGS) $(THREAD) 

client:
	$(CC) -I./header/*.h -o $(CLIENTOUT) $(SOURCEC)  $(CFLAGS) $(THREAD) 

all: server client

test1: server client
	./Test/test1
test2:  server client
	./Test/test2
test3: server client
	./Test/test3
clean:
	rm -f -R backup/*
	rm -f -R download/*
	rm -f  log/*
	rm -f socket/*

cleanall:
	rm -f $(SERVEROUT) $(CLIENTOUT) 