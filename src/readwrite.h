#ifndef READWRITE_H
#define READWRITE_H

#include <stdio.h>
#include <stdint.h>

extern uint16_t read_uint16_bigendian(FILE * file);
extern void write_uint16_bigendian(uint16_t x, FILE * file);
extern uint32_t read_uint32_bigendian(FILE * file);
extern void write_uint32_bigendian(uint32_t x, FILE * file);

#endif
