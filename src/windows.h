#ifndef WINDOWS_H
#define WINDOWS_H

#include "ncurses.h"
#include "yx_uint16.h"

struct Frame {
  WINDOW * curses_win;
  struct yx_uint16 size; };

struct WinMeta {
  WINDOW * screen;
  uint16_t pad_offset;
  struct Frame pad;
  struct Win * chain_start;
  struct Win * chain_end;
  struct Win * active; };

struct Win {
  struct Win * prev;
  struct Win * next;
  struct yx_uint16 start;
  struct Frame frame;
  char * title;
  void (* draw) (struct Win *);
  void * data; };

extern struct WinMeta init_win_meta (WINDOW *);
extern struct Win init_win (struct WinMeta *, char *, void *, void *);
extern void append_win (struct WinMeta *, struct Win *);
extern void suspend_win (struct WinMeta *, struct Win *);
extern void draw_scroll_hint (struct Frame *, uint16_t, uint32_t, char);
extern void draw_all_wins (struct WinMeta *);
extern void resize_active_win (struct WinMeta *, uint16_t, uint16_t);
extern void cycle_active_win (struct WinMeta *, char);
extern void shift_active_win (struct WinMeta *, char);
extern void reset_pad_offset (struct WinMeta *, uint16_t);

#endif
