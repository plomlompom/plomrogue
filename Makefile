roguelike:
	cc -g -o windows.o -c windows.c
	cc -g -o roguelike.o -c roguelike.c
	cc -g -o roguelike windows.o roguelike.o -lncurses -lm

clean:
	rm *.o; rm roguelike
