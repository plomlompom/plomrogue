struct WinMeta {
  struct Win * chain_start;
  struct Win * chain_end;
  struct Win * active;
  int height; };

struct Win {
  struct Win * prev;
  struct Win * next;
  int width;
  int height;
  int start_x;
  int start_y;
  WINDOW * curses_win;
  char border_left;
  char border_down;
  char * title;
  void (* draw) (struct Win *);
  void * data; };

struct  WinMeta init_win_meta (WINDOW *);
struct Win init_window (struct WinMeta *, char *);
void append_window (struct WinMeta *, struct Win *);
void suspend_window (struct WinMeta *, struct Win *);
void place_window (struct WinMeta *, struct Win *);
void update_windows (struct WinMeta *, struct Win *);
void destroy_window (struct Win *);
void draw_windows (struct WinMeta *, struct Win *);
void draw_all_windows (struct WinMeta *);
void draw_window(struct WinMeta *, struct Win *);
void undraw_window (WINDOW *);
void resize_window (struct WinMeta *, char);
void cycle_active_window (struct WinMeta *, char);
void shift_window (struct WinMeta *, char);
