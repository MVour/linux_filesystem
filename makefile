CFLAGS = -Wall -g

all:	main\
		MDS.o\
		list.o\
		functions.o\

main: main.o MDS.o list.o functions.o
	gcc -Wall -o main main.o MDS.o list.o functions.o

# mds: mds.o
# 	gcc -o mds mds.o

# list: list.o
# 	gcc -o list list.o

# functs: functs.o
# 	gcc -Wall -o functs functs.o

#######################

main.o: main.c MDS.h list.h 
	gcc $(CFLAGS) -c main.c MDS.c

MDS.o: MDS.c MDS.h
	gcc $(CFLAGS) -c MDS.c

list.o: list.c list.h
	gcc $(CFLAGS) -c list.c

functions.o: functions.c functions.h
	gcc $(CFLAGS) -c functions.c

# functs.o:
# 	gcc -Wall -c functions.c

#######################

.Phony: clean

clean:
	rm main *.o *.cfs