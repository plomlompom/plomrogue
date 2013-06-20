#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#define UCHARSIZE (UCHAR_MAX + 1)

uint16_t read_uint16_bigendian(FILE * file) {
// Read uint16 from file in big-endian order.
  unsigned char a = fgetc(file);
  unsigned char b = fgetc(file);
  return (a * UCHARSIZE) + b; }

void write_uint16_bigendian(uint16_t x, FILE * file) {
// Write uint16 to file in beg-endian order.
  unsigned char a = x / UCHARSIZE;
  unsigned char b = x % UCHARSIZE;
  fputc(a, file);
  fputc(b, file); }

uint32_t read_uint32_bigendian(FILE * file) {
// Read uint32 from file in big-endian order.
  unsigned char a = fgetc(file);
  unsigned char b = fgetc(file);
  unsigned char c = fgetc(file);
  unsigned char d = fgetc(file);
  return (a * UCHARSIZE * UCHARSIZE * UCHARSIZE) + (b * UCHARSIZE * UCHARSIZE) + (c * UCHARSIZE) + d; }

void write_uint32_bigendian(uint32_t x, FILE * file) {
// Write uint32 to file in beg-endian order.
  unsigned char a = x / (UCHARSIZE * UCHARSIZE * UCHARSIZE);
  unsigned char b = (x - (a * UCHARSIZE * UCHARSIZE * UCHARSIZE)) / (UCHARSIZE * UCHARSIZE);
  unsigned char c = (x - ((a * UCHARSIZE * UCHARSIZE * UCHARSIZE) + (b * UCHARSIZE * UCHARSIZE))) / UCHARSIZE;
  unsigned char d = x % UCHARSIZE;
  fputc(a, file);
  fputc(b, file);
  fputc(c, file);
  fputc(d, file); }
