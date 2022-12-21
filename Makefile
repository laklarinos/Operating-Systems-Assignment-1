erg1: parent.o helpers.o
	@gcc -o erg1 parent.o helpers.o -lpthread

parent.o: parent.c helpers.c child.c
	@gcc -c parent.c child.c helpers.c

child: child.o helpers.o
	@gcc -o child child.o helpers.o -lpthread

child.o: child.c helpers.c
	@gcc -c child.c helpers.c

helpers.o: helpers.c
	@gcc -c helpers.c 
run:
	./erg1 input_file.txt 10 10 100

valgrind:

	valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes ./erg1 input_file.txt 10 10 100

clean:
	@rm -f *.o child parent erg1 *.log

cleanl:
	@rm -f *.log

cleans:
	@rm -f dev/shm sem*