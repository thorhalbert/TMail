CC = gcc
OBJS = tmail.o malib.o profiles.o

tmail: $(OBJS)
	$(CC) -o tmail $(OBJS)
