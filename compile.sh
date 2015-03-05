#!/bin/sh
gcc -shared -fPIC -std=c11 -pedantic-errors -Wall -Werror -Wextra -Wformat-security -g -o libplomrogue.so libplomrogue.c
