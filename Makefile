TARGET=roguelike
SRCDIR=src
BUILDDIR=build
CC=gcc
CFLAGS=-Wall -g

# Use all .c files below SRCDIR for sources. Build object file paths by
# replacing SRCDIR with BUILDDIR and .c endings with .o endings.
SOURCES=$(shell find $(SRCDIR)/ -type f -name \*.c)
OBJECTS=$(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.c=.o))

# Linking TARGET demands generation of all OBJECTS files. To build them,
# rely on this rule: Each file of path BUILDDIR/[name].o is compiled
# from a file of path SRCDIR/[name].c. Build BUILDDIR as needed.
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o roguelike $(OBJECTS) -lncurses
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(BUILDDIR); $(CC) $(CFLAGS) -c $< -o $@

# Use "clean" to delete build and target files. Tell make that "clean"
# is a "phony" target, i.e. this is not about building a file.
.PHONY: clean
clean:
	rm $(OBJECTS)
	rmdir $(BUILDDIR)
	rm $(TARGET)
