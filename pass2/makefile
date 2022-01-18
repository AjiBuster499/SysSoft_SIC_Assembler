project2:	main.o symbols.o directives.o instructions.o
	gcc -o project2 -Wall -O0 main.o symbols.o directives.o instructions.o

main.o:	main.c
	gcc -c -g -Wall -O0 main.c

symbols.o:	symbols.c
	gcc -c -g -Wall -O0 symbols.c

directives.o:	directives.c
	gcc -c -g -Wall -O0 directives.c

instructions.o: instructions.c
	gcc -c -g -Wall -O0 instructions.c

clean:
	rm *.o -f
	rm *.obj -f
	touch *.c
	rm project2 -f
