CC=gcc
CFLAGS=-Wall -g
DFLAGS=-lpthread -Wall -g
TCP: newtcpclient.c newtcpserver.c
	$(CC) -o client $(DFLAGS) newtcpclient.c
	$(CC) -o server $(CFLAGS) newtcpserver.c

clean:
	$(RM) server client

