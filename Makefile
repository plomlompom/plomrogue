roguelike:
	cc -Wall -g -o windows.o -c windows.c
	cc -Wall -g -o draw_wins.o -c draw_wins.c
	cc -Wall -g -o keybindings.o -c keybindings.c
	cc -Wall -g -o roguelike.o -c roguelike.c
	cc -Wall -g -o roguelike windows.o draw_wins.o keybindings.o roguelike.o -lncurses

clean:
	rm *.o; rm roguelike
