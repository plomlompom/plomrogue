struct WinMeta {
  WINDOW * screen;
  WINDOW * pad;
  int pad_offset;
  struct Win * chain_start;
  struct Win * chain_end;
  struct Win * active;
  int width;
  int height; };

struct Win {
  struct Win * prev;
  struct Win * next;
  int width;
  int height;
  WINDOW * curses;
  char * title;
  void (* draw) (struct Win *);
  void * data; };

struct yx {
  int y;
  int x; };

struct Corners {
  struct yx tl;
  struct yx tr;
  struct yx bl;
  struct yx br; };

struct  WinMeta init_win_meta (WINDOW *);
struct Win init_window (struct WinMeta *, char *);
void append_window (struct WinMeta *, struct Win *);
void suspend_window (struct WinMeta *, struct Win *);
struct yx place_window (struct WinMeta *, struct Win *);
void update_windows (struct WinMeta *, struct Win *);
void destroy_window (struct Win *);
void draw_windows (struct Win *);
void draw_windows_borders (struct Win *, struct Win *, struct Corners *, int);
void draw_window_borders (struct Win *, char);
void draw_all_windows (struct WinMeta *);
void resize_window (struct WinMeta *, char);
void cycle_active_window (struct WinMeta *, char);
void shift_window (struct WinMeta *, char);
