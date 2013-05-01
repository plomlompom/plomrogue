#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include "windows.h"

struct WinMeta init_win_meta (WINDOW * screen) {
// Create and populate WinMeta struct with sane default values.
  struct WinMeta win_meta;
  win_meta.screen = screen;
  win_meta.height = screen->_maxy + 1;
  win_meta.width = screen->_maxx + 1;
  win_meta.chain_start = 0;
  win_meta.chain_end = 0;
  win_meta.pad_offset = 0;
  win_meta.pad = newpad(win_meta.height, 1);
  return win_meta; }

struct Win init_window (struct WinMeta * win_meta, char * title) {
// Create and populate Win struct with sane default values.
  struct Win win;
  win.prev = 0;
  win.next = 0;
  win.curses_win = 0;
  win.title = title;
  win.width = 20;
  win.height = win_meta->height - 1;
  return win; }

void append_window (struct WinMeta * win_meta, struct Win * win) {
// Append win to window chain. Set active, if first window. Update geometry of windows from new window on.
  if (0 != win_meta->chain_start) {
    win->prev = win_meta->chain_end;
    win_meta->chain_end->next = win; }
  else {
    win_meta->active = win;
    win_meta->chain_start = win; }
  win_meta->chain_end = win;
  update_windows(win_meta, win); }

void suspend_window (struct WinMeta * win_meta, struct Win * win) {
// Destroy win, suspend from window chain. Update geometry of following rows, as well as activity selection.
  destroy_window(win);
  if (win_meta->chain_start != win) // Give win's position in the chain to element next to it in the chain.
    win->prev->next = win->next;
  else
    win_meta->chain_start = win->next;
  if (win_meta->chain_end != win) { // Let chain element next to win know its new predecessor.
    win->next->prev = win->prev;
    if (win_meta->active == win)    // If win was active, shift active window pointer to ...
      win_meta->active = win->next;                     // ... the next chain element, if that is a window ...
    update_windows(win_meta, win->next); }
  else {
    win_meta->chain_end = win->prev;
    if (win_meta->active == win)                                   // ... or else to the previous element.
      win_meta->active = win->prev; }
  win->prev = 0;
  win->next = 0; }

struct yx place_window (struct WinMeta * win_meta, struct Win * win) {
// Based on position and sizes of previous window, find fitting place for current window.
  struct yx start;
  start.x = 0;                                     // if window is first in chain, place it on top-left corner
  start.y = 1;
  if (0 != win->prev) {
    if (win->prev->height == win_meta->height - 1)                      // if prev window fills entire column,
      start.x = getbegx(win->prev->curses_win) + win->prev->width + 1;  // place win in new column next to it
    else {
      struct Win * first_ceiling = win->prev;                         // first_ceiling determines column with;
      while (getbegy(first_ceiling->curses_win) != 1)                 // default: place window in new column
        first_ceiling = first_ceiling->prev;                          // next to it
      start.x = getbegx(first_ceiling->curses_win) + first_ceiling->width + 1;
      if (first_ceiling->width >= win->width) {      // only place wins in prev column that fit into its width
        struct Win * win_p = first_ceiling;
        struct Win * lastrow_startwin = win_p;
        while (win_p != win) {
          if (getbegx(win_p->curses_win) == getbegx(first_ceiling->curses_win))
            lastrow_startwin = win_p;               // try to fit window at the end of the last row of windows
          win_p = win_p ->next; }                   // inside column; if failure, try opening a new row below
        int lastcol_start = getbegx(win->prev->curses_win) + win->prev->width + 1;
        if (win->width <= getbegx(first_ceiling->curses_win) + first_ceiling->width - lastcol_start
            && win->height <= lastrow_startwin->height) {
          start.x = lastcol_start;
          start.y = getbegy(lastrow_startwin->curses_win); }
        else if (win->height < win_meta->height - (getbegy(lastrow_startwin->curses_win) + lastrow_startwin->height)
                 && win->width <= first_ceiling->width) {
          start.x = getbegx(first_ceiling->curses_win);
          start.y = getbegy(lastrow_startwin->curses_win) + lastrow_startwin->height + 1; } } } }
  return start; }

void update_windows (struct WinMeta * win_meta, struct Win * win) {
// Update geometry of win and its next of kin. Destroy (if visible), (re-)build window. If need, resize pad.
  if (0 != win->curses_win)
    destroy_window (win);
  struct yx startyx = place_window(win_meta, win);
  int lastwincol = 0;
  struct Win * win_p = win_meta->chain_start;
  while (win_p != 0) {
    if (win_p != win && getbegx(win_p->curses_win) + win_p->width > lastwincol + 1)
      lastwincol = getbegx(win_p->curses_win) + win_p->width - 1;
    else if (win_p == win && startyx.x + win->width > lastwincol + 1)
      lastwincol = startyx.x + win->width - 1;
    win_p = win_p->next; }
  if (getmaxx(win_meta->pad) != lastwincol) {
    wresize(win_meta->pad, getmaxy(win_meta->pad), lastwincol + 2); }
  win->curses_win = subpad(win_meta->pad, win->height, win->width, startyx.y, startyx.x);
  if (0 != win->next)
    update_windows (win_meta, win->next); }

void destroy_window (struct Win * win) {
// Delete window.
  delwin(win->curses_win);
  win->curses_win = 0; }

void draw_window_borders (struct Win * win, char active) {
// Draw borders of window win, including title. Decorate in a special way if window is marked as active.
  int y, x;
  for (y = getbegy(win->curses_win); y <= getbegy(win->curses_win) + win->height; y++) {
    mvwaddch(wgetparent(win->curses_win), y, getbegx(win->curses_win) - 1, '|');
    mvwaddch(wgetparent(win->curses_win), y, getbegx(win->curses_win) + win->width, '|'); }
  for (x = getbegx(win->curses_win); x <= getbegx(win->curses_win) + win->width; x++) {
    mvwaddch(wgetparent(win->curses_win), getbegy(win->curses_win) - 1, x, '-');
    mvwaddch(wgetparent(win->curses_win), getbegy(win->curses_win) + win->height, x, '-'); }
  char min_title_length_visible = 3; // 1 char minimal, plus 2 chars for decoration left/right of title
  if (win->width > min_title_length_visible) {
    int title_length = strlen(win->title);
    int title_offset = (((win->width) - (title_length + 2)) / 2); // + 2 is for decoration
    int length_visible = strnlen(win->title, win->width - min_title_length_visible);
    char title[length_visible + 3];
    char decoration = ' ';
    if (1 == active)
      decoration = '$';
    memcpy(title + 1, win->title, length_visible);
    title[0] = title[length_visible + 1] = decoration;
    title[length_visible + 2] = '\0';
    mvwaddstr(wgetparent(win->curses_win), getbegy(win->curses_win) - 1, getbegx(win->curses_win) + title_offset, title); } }

void draw_windows_borders (struct Win * win, struct Win * win_active, struct Corners * corners, int ccount) {
// Craw draw_window_borders() for all windows in chain from win on. Save current window's border corners.
  char active = 0;
  if (win == win_active)
    active = 1;
   draw_window_borders(win, active);
  corners[ccount].tl.y = getbegy(win->curses_win) - 1;
  corners[ccount].tl.x = getbegx(win->curses_win) - 1;
  corners[ccount].tr.y = getbegy(win->curses_win) - 1;
  corners[ccount].tr.x = getbegx(win->curses_win) + win->width;
  corners[ccount].bl.y = getbegy(win->curses_win) + win->height;
  corners[ccount].bl.x = getbegx(win->curses_win) - 1;
  corners[ccount].br.y = getbegy(win->curses_win) + win->height;
  corners[ccount].br.x = getbegx(win->curses_win) + win->width;
  if (0 != win->next) {
    draw_windows_borders (win->next, win_active, corners, ccount + 1); } }

void draw_window(struct Win * win) {
// Draw window content if visible.
  if (win->height > 1 && win->width > 1) ;
    win->draw(win); }

void draw_windows (struct Win * win) {
// Draw contents of all windows in window chain from win on.
  draw_window(win);
  if (0 != win->next) {
    draw_windows (win->next); } }

void draw_all_windows (struct WinMeta * win_meta) {
// Draw all windows and their borders.
  erase();
  wnoutrefresh(win_meta->screen);
  werase(win_meta->pad);
  if (win_meta->chain_start) {
    int n_wins = 1;
    struct Win * win_p = win_meta->chain_start;
    while (0 != win_p->next) {
      win_p = win_p->next;
      n_wins++; }
    struct Corners * all_corners = malloc(sizeof(struct Corners) * n_wins);
    draw_windows (win_meta->chain_start);
    draw_windows_borders (win_meta->chain_start, win_meta->active, all_corners, 0);
    int i;
    for (i = 0; i < n_wins; i++) {
      mvwaddch(win_meta->pad, all_corners[i].tl.y, all_corners[i].tl.x, '+');
      mvwaddch(win_meta->pad, all_corners[i].tr.y, all_corners[i].tr.x, '+');
      mvwaddch(win_meta->pad, all_corners[i].bl.y, all_corners[i].bl.x, '+');
      mvwaddch(win_meta->pad, all_corners[i].br.y, all_corners[i].br.x, '+'); }
    pnoutrefresh(win_meta->pad, 0, win_meta->pad_offset, 0, 0, win_meta->height, win_meta->width - 1);
    free(all_corners); }
  doupdate(); }

void resize_window (struct WinMeta * win_meta, char change) {
// Grow or shrink currently active window. Correct its geometry and that of its followers.
  if      (change == '-' && win_meta->active->height > 1)
      win_meta->active->height--;
  else if (change == '+' && win_meta->active->height < win_meta->height - 1)
    win_meta->active->height++;
  else if (change == '_' && win_meta->active->width > 1)
      win_meta->active->width--;
  else if (change == '*')
    win_meta->active->width++;
  update_windows(win_meta, win_meta->chain_start); }

void cycle_active_window (struct WinMeta * win_meta, char dir) {
// Cycle active window selection forwards (dir = 'n') or backwards.
  if ('n' == dir) {
    if (win_meta->active->next != 0)
      win_meta->active = win_meta->active->next;
    else
      win_meta->active = win_meta->chain_start; }
  else {
    if (win_meta->active->prev != 0)
      win_meta->active = win_meta->active->prev;
    else
      win_meta->active = win_meta->chain_end; } }

void shift_window (struct WinMeta * win_meta, char dir) {
// Move active window forward/backward in window chain. If jumping beyond start/end, move to other chain end.
  if (win_meta->chain_start != win_meta->chain_end && (dir == 'f' || dir == 'b')) {
    int i, i_max;
    struct Win * win_shift = win_meta->active;
    char wrap = 0;
    if ((dir == 'f' && win_shift == win_meta->chain_end)
        || (dir == 'b' && win_shift == win_meta->chain_start))
      wrap = 1;
    struct Win * win_p, * win_p_next;
    for (i_max = 1, win_p = win_meta->chain_start; win_p != win_meta->chain_end; i_max++, win_p = win_p->next);
    struct Win ** wins = malloc(i_max * sizeof(struct Win *));
    for (i = 0, win_p = win_meta->chain_start; i < i_max; i++) {
      win_p_next = win_p->next;
      suspend_window(win_meta, win_p);
      wins[i] = win_p;
      win_p = win_p_next; }
    if (wrap)
      if (dir == 'f') {
        append_window(win_meta, win_shift);
        for (i = 0; i < i_max - 1; i++)
          append_window(win_meta, wins[i]); }
      else {
        for (i = 1; i < i_max; i++)
          append_window(win_meta, wins[i]);
        append_window(win_meta, win_shift); }
    else
      for (i = 0; i < i_max; i++)
        if ((dir == 'f' && win_shift == wins[i]) || (dir == 'b' && win_shift == wins[i+1])) {
          append_window(win_meta, wins[i+1]);
          append_window(win_meta, wins[i]);
          i++; }
        else
          append_window(win_meta, wins[i]);
    free(wins);
    win_meta->active = win_shift; } }
