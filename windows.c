#include <stdlib.h>
#include <stdint.h>
#include <ncurses.h>
#include <string.h>
#include "windows.h"

struct Corners {
  struct yx_uint16 tl;
  struct yx_uint16 tr;
  struct yx_uint16 bl;
  struct yx_uint16 br; };

static void refit_pad (struct WinMeta *);
static void place_window (struct WinMeta *, struct Win *);
static void update_windows (struct WinMeta *, struct Win *);
static void destroy_window (struct Win *);
static void draw_wins_borders (struct Win *, struct Win *, struct Corners *, uint16_t);
static void draw_win_borders (struct Win *, char);
static void draw_windows (struct Win *);

extern struct WinMeta init_win_meta (WINDOW * screen) {
// Create and populate WinMeta struct with sane default values.
  struct WinMeta win_meta;
  win_meta.screen = screen;
  win_meta.pad.size.y = getmaxy(screen);
  win_meta.pad.size.x = getmaxx(screen);
  win_meta.chain_start = 0;
  win_meta.chain_end = 0;
  win_meta.pad_offset = 0;
  win_meta.pad.curses_win = newpad(win_meta.pad.size.y, 1);
  win_meta.active = 0;
  return win_meta; }

extern struct Win init_window (struct WinMeta * win_meta, char * title, void * data, void * func) {
// Create and populate Win struct with sane default values.
  struct Win win;
  win.prev = 0;
  win.next = 0;
  win.frame.curses_win = 0;
  win.title = title;
  win.frame.size.x = 20;
  win.frame.size.y = win_meta->pad.size.y - 1;
  win.data = data;
  win.draw = func;
  return win; }

extern void append_window (struct WinMeta * win_meta, struct Win * win) {
// Append win to window chain. Set active, if first window. Update geometry of windows from new window on.
  if (0 != win_meta->chain_start) {
    win->prev = win_meta->chain_end;
    win_meta->chain_end->next = win; }
  else {
    win_meta->active = win;
    win_meta->chain_start = win; }
  win_meta->chain_end = win;
  update_windows(win_meta, win); }

static void refit_pad (struct WinMeta * win_meta) {
// Fit pad width to minimum width demanded by current windows' geometries.
  uint16_t lastwincol = 0;
  struct Win * win_p = win_meta->chain_start;
  while (win_p != 0) {
    if (win_p->start.x + win_p->frame.size.x > lastwincol + 1)
      lastwincol = win_p->start.x + win_p->frame.size.x - 1;
    win_p = win_p->next; }
  if (getmaxx(win_meta->pad.curses_win) != lastwincol)
    wresize(win_meta->pad.curses_win, getmaxy(win_meta->pad.curses_win), lastwincol + 2); }

extern void suspend_window (struct WinMeta * win_meta, struct Win * win) {
// Destroy win, suspend from chain. Update geometry of following rows and pad, as well as activity selection.
  destroy_window(win);
  if (win_meta->chain_start != win)    // Give win's position in the chain to element next to it in the chain.
    win->prev->next = win->next;
  else
    win_meta->chain_start = win->next;
  char pad_refitted = 0;
  if (win_meta->chain_end != win) {                 // Let chain element next to win know its new predecessor.
    win->next->prev = win->prev;
    if (win_meta->active == win)                          // If win was active, shift active window pointer to
      win_meta->active = win->next;                       // the next chain element, if that is a window ...
    update_windows(win_meta, win->next);
    pad_refitted = 1; }
  else {
    win_meta->chain_end = win->prev;
    if (win_meta->active == win)                                       // ... or else to the previous element.
      win_meta->active = win->prev; }
  win->prev = 0;
  win->next = 0;
  if (0 == pad_refitted)                                                            // Refit pad if necessary.
    refit_pad(win_meta); }

static void place_window (struct WinMeta * win_meta, struct Win * w) {
// Based on position and sizes of previous window, find fitting place for current window.
  w->start.x = 0;                                  // if window is first in chain, place it on top-left corner
  w->start.y = 1;
  if (0 != w->prev) {
    struct Win * w_top = w->prev;
    while (w_top->start.y != 1)
      w_top = w_top->prev;                                       // else, default to placing window in new top
    w->start.x = w_top->start.x + w_top->frame.size.x + 1;       // column to the right of the last one
    uint16_t w_prev_maxy = w->prev->start.y + getmaxy(w->prev->frame.curses_win);
    if (w->frame.size.x <= w->prev->frame.size.x && w->frame.size.y < win_meta->pad.size.y - w_prev_maxy) {
      w->start.x = w->prev->start.x;                       // place window below previous window if it fits
      w->start.y = w_prev_maxy + 1; }                      // vertically and is not wider than its predecessor
    else {
      struct Win * w_up = w->prev;
      struct Win * w_upup = w_up;
      uint16_t widthdiff;
      while (w_up != w_top) {
        w_upup = w_up->prev;
        while (1) {
          if (w_up->start.y != w_upup->start.y)
            break;
          w_upup = w_upup->prev; }
        w_prev_maxy = w_upup->start.y + getmaxy(w_upup->frame.curses_win);
        widthdiff = (w_upup->start.x + w_upup->frame.size.x) - (w_up->start.x + w_up->frame.size.x);
        if (w->frame.size.y < win_meta->pad.size.y - w_prev_maxy && w->frame.size.x < widthdiff) {
          w->start.x = w_up->start.x + w_up->frame.size.x + 1 ; // else try to open new sub column under
          w->start.y = w_prev_maxy + 1;                        // last window below which enough space remains
          break; }
        w_up = w_upup; } } } }

static void update_windows (struct WinMeta * wmeta, struct Win * w) {
// Update geometry of win and its next of kin. Destroy (if visible), (re-)build window. If need, resize pad.
  if (0 != w->frame.curses_win)
    destroy_window (w);
  place_window(wmeta, w);
  refit_pad(wmeta);
  w->frame.curses_win=subpad(wmeta->pad.curses_win, w->frame.size.y, w->frame.size.x, w->start.y, w->start.x);
  if (0 != w->next)
    update_windows (wmeta, w->next); }

static void destroy_window (struct Win * win) {
// Delete window.
  delwin(win->frame.curses_win);
  win->frame.curses_win = 0; }

static void draw_win_borders (struct Win * win, char active) {
// Draw borders of window win, including title. Decorate in a special way if window is marked as active.
  uint16_t y, x;
  for (y = win->start.y; y <= win->start.y + win->frame.size.y; y++) {
    mvwaddch(wgetparent(win->frame.curses_win), y, win->start.x - 1, '|');
    mvwaddch(wgetparent(win->frame.curses_win), y, win->start.x + win->frame.size.x, '|'); }
  for (x = win->start.x; x <= win->start.x + win->frame.size.x; x++) {
    mvwaddch(wgetparent(win->frame.curses_win), win->start.y - 1, x, '-');
    mvwaddch(wgetparent(win->frame.curses_win), win->start.y + win->frame.size.y, x, '-'); }
  char min_title_length_visible = 3;        // 1 char minimal, plus 2 chars for decoration left/right of title
  if (win->frame.size.x >= min_title_length_visible) {
    uint16_t title_offset = 0;
    if (win->frame.size.x > strlen(win->title) + 2)
      title_offset = (win->frame.size.x - (strlen(win->title) + 2)) / 2;              // + 2 is for decoration
    uint16_t length_visible = strnlen(win->title, win->frame.size.x - 2);
    char title[length_visible + 3];
    char decoration = ' ';
    if (1 == active)
      decoration = '$';
    memcpy(title + 1, win->title, length_visible);
    title[0] = title[length_visible + 1] = decoration;
    title[length_visible + 2] = '\0';
    mvwaddstr(wgetparent(win->frame.curses_win), win->start.y - 1, win->start.x + title_offset, title); } }

static void draw_wins_borders (struct Win * w, struct Win * w_active, struct Corners * corners, uint16_t i) {
// Call draw_win_borders() for all windows in chain from win on. Save current window's border corners.
  char active = 0;
  if (w == w_active)
    active = 1;
  draw_win_borders(w, active);
  corners[i].tl.y = w->start.y - 1;
  corners[i].tl.x = w->start.x - 1;
  corners[i].tr.y = w->start.y - 1;
  corners[i].tr.x = w->start.x + w->frame.size.x;
  corners[i].bl.y = w->start.y + w->frame.size.y;
  corners[i].bl.x = w->start.x - 1;
  corners[i].br.y = w->start.y + w->frame.size.y;
  corners[i].br.x = w->start.x + w->frame.size.x;
  if (0 != w->next) {
    draw_wins_borders (w->next, w_active, corners, i + 1); } }

static void draw_windows (struct Win * win) {
// Draw contents of all windows in window chain from win on.
  win->draw(win);
  if (0 != win->next) {
    draw_windows (win->next); } }

extern void draw_scroll_hint (struct Frame * frame, uint16_t pos, uint32_t dist, char dir) {
// Draw scroll hint into frame at pos (row or col dependend on dir), mark distance of dist cells into dir.
  char more[] = "more";
  char unit_cols[] = "columns";
  char unit_rows[] = "lines";
  uint16_t dsc_space = frame->size.x;
  char * unit = unit_rows;
  if ('<' == dir || '>' == dir) {
    dsc_space = frame->size.y;
    unit = unit_cols; }
  char * scrolldsc = malloc((4 * sizeof(char)) + strlen(more) + strlen(unit) + 10);  // 10 = uint32 max strlen
  sprintf(scrolldsc, " %d %s %s ", dist, more, unit);
  char offset = 1, q;
  if (dsc_space > strlen(scrolldsc) + 1)
    offset = (dsc_space - strlen(scrolldsc)) / 2;
  chtype symbol;
  for (q = 0; q < dsc_space; q++) {
    if (q >= offset && q < strlen(scrolldsc) + offset)
      symbol = scrolldsc[q - offset] | A_REVERSE;
    else
      symbol = dir | A_REVERSE;
    if ('<' == dir || '>' == dir)
      mvwaddch(frame->curses_win, q, pos, symbol);
    else
      mvwaddch(frame->curses_win, pos, q, symbol); }
  free(scrolldsc); }

extern void draw_all_windows (struct WinMeta * wmeta) {
// Draw pad with all windows and their borders, plus scrolling hints.
  erase();
  wnoutrefresh(wmeta->screen);
  werase(wmeta->pad.curses_win);
  if (wmeta->chain_start) {
    uint16_t n_wins = 1, i, y;
    struct Win * win_p = wmeta->chain_start;
    while (0 != win_p->next) {
      win_p = win_p->next;
      n_wins++; }
    struct Corners * all_corners = malloc(sizeof(struct Corners) * n_wins);
    draw_windows (wmeta->chain_start);
    draw_wins_borders (wmeta->chain_start, wmeta->active, all_corners, 0);
    for (i = 0; i < n_wins; i++) {
      mvwaddch(wmeta->pad.curses_win, all_corners[i].tl.y, all_corners[i].tl.x, '+');
      mvwaddch(wmeta->pad.curses_win, all_corners[i].tr.y, all_corners[i].tr.x, '+');
      mvwaddch(wmeta->pad.curses_win, all_corners[i].bl.y, all_corners[i].bl.x, '+');
      mvwaddch(wmeta->pad.curses_win, all_corners[i].br.y, all_corners[i].br.x, '+'); }
    free(all_corners);
    if (wmeta->pad_offset > 0)
      draw_scroll_hint(&wmeta->pad, wmeta->pad_offset, wmeta->pad_offset + 1, '<');
    if (wmeta->pad_offset + wmeta->pad.size.x < getmaxx(wmeta->pad.curses_win) - 1)
      for (y = 0; y < wmeta->pad.size.y; y++)
        draw_scroll_hint(&wmeta->pad, wmeta->pad_offset + wmeta->pad.size.x - 1,
                         getmaxx(wmeta->pad.curses_win) - (wmeta->pad_offset + wmeta->pad.size.x), '>');
    pnoutrefresh(wmeta->pad.curses_win, 0, wmeta->pad_offset, 0, 0, wmeta->pad.size.y, wmeta->pad.size.x-1); }
  doupdate(); }

extern void resize_active_window (struct WinMeta * win_meta, uint16_t height, uint16_t width) {
// Grow or shrink currently active window. Correct its geometry and that of its followers.
  if (0 != win_meta->active && width > 0 && height > 0 && height < win_meta->pad.size.y) {
    win_meta->active->frame.size.y = height;
    win_meta->active->frame.size.x = width;
    update_windows(win_meta, win_meta->chain_start); } }

extern void cycle_active_window (struct WinMeta * win_meta, char dir) {
// Cycle active window selection forwards (dir = 'n') or backwards.
  if (0 != win_meta->active) {
    if ('n' == dir) {
      if (win_meta->active->next != 0)
        win_meta->active = win_meta->active->next;
      else
        win_meta->active = win_meta->chain_start; }
    else {
      if (win_meta->active->prev != 0)
        win_meta->active = win_meta->active->prev;
      else
        win_meta->active = win_meta->chain_end; } } }

extern void shift_active_window (struct WinMeta * win_meta, char dir) {
// Move active window forward/backward in window chain. If jumping beyond start/end, move to other chain end.
  if (0 != win_meta->active && win_meta->chain_start != win_meta->chain_end && (dir == 'f' || dir == 'b')) {
    struct Win * win_shift = win_meta->active, * win_p, * win_p_next;
    char wrap = 0;
    if ((dir == 'f' && win_shift == win_meta->chain_end)
        || (dir == 'b' && win_shift == win_meta->chain_start))
      wrap = 1;
    uint16_t i, i_max;
    for (i_max = 1, win_p = win_meta->chain_start; win_p != win_meta->chain_end; i_max++)
      win_p = win_p->next;
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

extern void reset_pad_offset(struct WinMeta * win_meta, uint16_t new_offset) {
// Apply new_offset to windows pad, if it proves to be sane.
  if (new_offset >= 0 && new_offset + win_meta->pad.size.x < getmaxx(win_meta->pad.curses_win))
    win_meta->pad_offset = new_offset; }
