struct WinMeta {
  WINDOW * screen;
  WINDOW * pad;
  uint16_t  pad_offset;
  struct Win * chain_start;
  struct Win * chain_end;
  struct Win * active;
  uint16_t width;
  uint16_t height; };

struct Win {
  struct Win * prev;
  struct Win * next;
  uint16_t startx;
  uint16_t width;
  uint16_t height;
  WINDOW * curses;
  char * title;
  void (* draw) (struct Win *);
  void * data; };

extern struct  WinMeta init_win_meta (WINDOW *);
extern struct Win init_window (struct WinMeta *, char *, void *, void *);
extern void append_window (struct WinMeta *, struct Win *);
extern void suspend_window (struct WinMeta *, struct Win *);
extern void draw_all_windows (struct WinMeta *);
extern void resize_active_window (struct WinMeta *, uint16_t, uint16_t);
extern void cycle_active_window (struct WinMeta *, char);
extern void shift_active_window (struct WinMeta *, char);
extern void reset_pad_offset (struct WinMeta *, uint16_t);
