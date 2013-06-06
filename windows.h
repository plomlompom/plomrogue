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
  uint16_t width;
  uint16_t height;
  WINDOW * curses;
  char * title;
  void (* draw) (struct Win *);
  void * data; };

struct yx {
  uint16_t y;
  uint16_t x; };

struct Corners {
  struct yx tl;
  struct yx tr;
  struct yx bl;
  struct yx br; };

struct  WinMeta init_win_meta (WINDOW *);
void scroll_pad (struct WinMeta *, char);
struct Win init_window (struct WinMeta *, char *, void *, void *);
void append_window (struct WinMeta *, struct Win *);
void suspend_window (struct WinMeta *, struct Win *);
struct yx place_window (struct WinMeta *, struct Win *);
void update_windows (struct WinMeta *, struct Win *);
void destroy_window (struct Win *);
void draw_windows (struct Win *);
void draw_windows_borders (struct Win *, struct Win *, struct Corners *, uint16_t);
void draw_window_borders (struct Win *, char);
void draw_vertical_scroll_hint (struct WinMeta *, uint16_t, uint32_t, char);
void draw_all_windows (struct WinMeta *);
void resize_active_window (struct WinMeta *, uint16_t, uint16_t);
void cycle_active_window (struct WinMeta *, char);
void shift_active_window (struct WinMeta *, char);
