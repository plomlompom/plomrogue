/*  readwrite.c */

#include "readwrite.h"
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

static const uint16_t uchar_s = UCHAR_MAX + 1;

extern uint16_t read_uint16_bigendian(FILE * file) {
  unsigned char a = fgetc(file);
  unsigned char b = fgetc(file);
  return (a * uchar_s) + b; }

extern uint32_t read_uint32_bigendian(FILE * file) {
  unsigned char a = fgetc(file);
  unsigned char b = fgetc(file);
  unsigned char c = fgetc(file);
  unsigned char d = fgetc(file);
  return (a * uchar_s * uchar_s * uchar_s) + (b * uchar_s * uchar_s) + (c * uchar_s) + d; }

extern void write_uint16_bigendian(uint16_t x, FILE * file) {
  unsigned char a = x / uchar_s;
  unsigned char b = x % uchar_s;
  fputc(a, file);
  fputc(b, file); }

extern void write_uint32_bigendian(uint32_t x, FILE * file) {
  unsigned char a = x / (uchar_s * uchar_s * uchar_s);
  unsigned char b = (x - (a * uchar_s * uchar_s * uchar_s)) / (uchar_s * uchar_s);
  unsigned char c = (x - ((a * uchar_s * uchar_s * uchar_s) + (b * uchar_s * uchar_s))) / uchar_s;
  unsigned char d = x % uchar_s;
  fputc(a, file);
  fputc(b, file);
  fputc(c, file);
  fputc(d, file); }
