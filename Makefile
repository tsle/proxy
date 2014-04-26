CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -lpthread

OBJS = proxy.o csapp.o token.o

all: proxy

proxy: $(OBJS)
	$(CC) $(OBJS) -o proxy $(LDFLAGS)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c
	$(CC) $(CFLAGS) -c proxy.c

token.o: token.c
	$(CC) $(CFLAGS) -c token.c

clean:
	rm -f *~ *.o proxy core

