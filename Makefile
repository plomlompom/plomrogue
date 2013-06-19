CC=cc
CFLAGS=-Wall -g
TARGET=roguelike
OBJECTS=windows.o draw_wins.o keybindings.o readwrite.o roguelike.o

roguelike: $(OBJECTS)
	$(CC) $(CFLAGS) -o roguelike $(OBJECTS) -lncurses

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm $(OBJECTS); rm roguelike
