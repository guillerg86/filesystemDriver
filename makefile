all: clean copycat
func.o: func.c func.h
	gcc -Wall -Wextra -c func.c
copycat.o: 
	gcc -Wall -Wextra -c copycat.c
copycat: copycat.o func.o
	gcc func.o copycat.o -o copycat
	rm -f func.o copycat.o
clean:
	rm -f func.o copycat.o

