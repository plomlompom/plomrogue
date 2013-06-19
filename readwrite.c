#include <stdio.h>
#include <limits.h>
#include <stdint.h>

uint16_t read_uint16_bigendian(FILE * file) {
// Read uint16 from file in big-endian order.
  const uint16_t nchar = UCHAR_MAX + 1;
  unsigned char a = fgetc(file);
  unsigned char b = fgetc(file);
  return (a * nchar) + b; }

void write_uint16_bigendian(uint16_t x, FILE * file) {
// Write uint16 to file in beg-endian order.
  const uint16_t nchar = UCHAR_MAX + 1;
  unsigned char a = x / nchar;
  unsigned char b = x % nchar;
  fputc(a, file);
  fputc(b, file); }

uint32_t read_uint32_bigendian(FILE * file) {
// Read uint32 from file in big-endian order.
  const uint16_t nchar = UCHAR_MAX + 1;
  unsigned char a = fgetc(file);
  unsigned char b = fgetc(file);
  unsigned char c = fgetc(file);
  unsigned char d = fgetc(file);
  return (a * nchar * nchar * nchar) + (b * nchar * nchar) + (c * nchar) + d; }

void write_uint32_bigendian(uint32_t x, FILE * file) {
// Write uint32 to file in beg-endian order.
  const uint16_t nchar = UCHAR_MAX + 1;
  unsigned char a = x / (nchar * nchar * nchar);
  unsigned char b = (x - (a * nchar * nchar * nchar)) / (nchar * nchar);
  unsigned char c = (x - ((a * nchar * nchar * nchar) + (b * nchar * nchar))) / nchar;
  unsigned char d = x % nchar;
  fputc(a, file);
  fputc(b, file);
  fputc(c, file);
  fputc(d, file); }

