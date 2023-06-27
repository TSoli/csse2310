CC=gcc
CFLAGS= -Wall -pedantic -std=gnu99 -pthread -I/local/courses/csse2310/include -g
LIBCFLAGS= -fPIC -Wall -pedantic -std=gnu99 -I/local/courses/csse2310/include
LDFLAGS= -L/local/courses/csse2310/lib -lcsse2310a3 -lcsse2310a4 -lstringstore



.PHONY: all 
.DEFAULT_GOAL := all

CLIENT_OBJS=dbclient.o readCommline.o utilities.o
SERVER_OBJS=dbserver.o readCommline.o utilities.o

all: dbclient dbserver libstringstore.so

dbclient: $(CLIENT_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o dbclient $(CLIENT_OBJS)

dbserver: $(SERVER_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o dbserver $(SERVER_OBJS)

libstringstore.so: stringstore.o
	$(CC) -shared -o $@ stringstore.o

stringstore.o: stringstore.c
	$(CC) $(LIBCFLAGS) -c $<

clean:
	rm dbclient *.o

