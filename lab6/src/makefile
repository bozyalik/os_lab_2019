CC=gcc
CFLAGS=-pthread -Wall

LIBS=libModule.a

all: server client $(LIBS)

$(LIBS): libModule.o
	ar rcs $@ $^

server: server.c $(LIBS)
	$(CC) -o server server.c  -L. -lModule $(CFLAGS)

client: client.c $(LIBS)
	$(CC) -o client client.c  -L. -lModule $(CFLAGS)

client.o: client.c libModule.h
	$(CC) -c client.c $(CFLAGS)

server.o: server.c libModule.h
	$(CC) -c server.c $(CFLAGS)

libModule.o: libModule.c libModule.h
	$(CC) -o libModule.o -c libModule.c $(CFLAGS)

clean:
	rm -f *.o client server $(LIBS)
