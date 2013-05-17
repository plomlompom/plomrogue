roguelike:
	cc -Wall -g -o windows.o -c windows.c
	cc -Wall -g -o roguelike.o -c roguelike.c
	cc -Wall -g -o roguelike windows.o roguelike.o -lncurses

clean:
	rm *.o; rm roguelike
