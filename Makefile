CC=gcc
CFLAGS=-Wall -Wextra -Werror -Wformat-security -g
TARGET_SERVER=roguelike-server
TARGET_CLIENT=roguelike-client
SRCDIR=src
BUILDDIR=build
SERVERDIR=server
CLIENTDIR=client
COMMONDIR=common

# Build object file lists by building object paths from all source file paths.
SOURCES_SERVER=$(shell find $(SRCDIR)/$(SERVERDIR)/ -type f -name \*.c)
SOURCES_CLIENT=$(shell find $(SRCDIR)/$(CLIENTDIR)/ -type f -name \*.c)
SOURCES_COMMON=$(shell find $(SRCDIR)/$(COMMONDIR)/ -type f -name \*.c)
OBJECTS_SERVER=$(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES_SERVER:.c=.o))
OBJECTS_CLIENT=$(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES_CLIENT:.c=.o))
OBJECTS_COMMON=$(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES_COMMON:.c=.o))

# "make" without further specifications builds both server and client.
default : $(TARGET_SERVER) $(TARGET_CLIENT)

# "make roguelike-server" builds only the server.
$(TARGET_SERVER) : $(OBJECTS_SERVER) $(OBJECTS_COMMON)
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(OBJECTS_SERVER) $(OBJECTS_COMMON)

# "make roguelike-client" builds only the ncurses client.
$(TARGET_CLIENT) : $(OBJECTS_CLIENT) $(OBJECTS_COMMON)
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(OBJECTS_CLIENT) $(OBJECTS_COMMON)\
	 -lncurses

# Build respective object file to any source file. Create build dirs as needed.
$(BUILDDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# "make clean" to tries to delete all files that could possibly have been built.
# Declare target "phony", i.e. this is not about building a file.
.PHONY : clean
clean :
	rm -rf $(BUILDDIR)
	rm -f $(TARGET_SERVER)
	rm -f $(TARGET_CLIENT)
