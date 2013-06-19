CC=cc
CFLAGS=-Wall -g
TARGET=roguelike
SOURCES=$(shell find . -type f -name \*.c)
OBJECTS=$(SOURCES:.c=.o)

roguelike: $(OBJECTS)
	$(CC) $(CFLAGS) -o roguelike $(OBJECTS) -lncurses

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm $(OBJECTS); rm roguelike
