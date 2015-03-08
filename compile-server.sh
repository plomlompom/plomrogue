#!/bin/sh
gcc -shared -fPIC -std=c11 -pedantic-errors -Wall -Werror -Wextra -Wformat-security -O3 -o libplomrogue.so libplomrogue.c
