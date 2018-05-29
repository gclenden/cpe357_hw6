CC = gcc
CFLAGS =-Wall -g
MAIN = mush
OBJS = mush.o parseline.o

all : $(MAIN)

$(MAIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

parseline.o: parseline.c parseline.h
	$(CC) $(CFLAGS) -c parseline.c

mush.o: mush.c mush.h
	$(CC) $(CFLAGS) -c mush.c

clean :
	rm *.o $(MAIN)

handin:
	handin getaylor-grader 357hw6-11 handindir/*

test: 
	~getaylor-grader/tryAsgn6
