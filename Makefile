CC=cc
CFLAGS=-Wall -g
TARGET=roguelike
SRCDIR=src
BUILDDIR=build
SOURCES=$(shell find $(SRCDIR)/ -type f -name \*.c)
OBJECTS=$(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.c=.o))

roguelike: $(OBJECTS)
	$(CC) $(CFLAGS) -o roguelike $(OBJECTS) -lncurses

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(BUILDDIR); $(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm $(OBJECTS)
	rmdir $(BUILDDIR)
	rm $(TARGET)
