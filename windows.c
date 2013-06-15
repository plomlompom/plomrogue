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
static void draw_windows_borders (struct Win *, struct Win *, struct Corners *, uint16_t);
static void draw_window_borders (struct Win *, char);
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

static void place_window (struct WinMeta * win_meta, struct Win * win) {
// Based on position and sizes of previous window, find fitting place for current window.
  win->start.x = 0;                                // if window is first in chain, place it on top-left corner
  win->start.y = 1;
  if (0 != win->prev) {
    struct Win * win_top = win->prev;
    while (win_top->start.y != 1)
      win_top = win_top->prev;                                   // else, default to placing window in new top
    win->start.x = win_top->start.x + win_top->frame.size.x + 1; // column to the right of the last one
    uint16_t winprev_maxy = win->prev->start.y + getmaxy(win->prev->frame.curses_win);
    if (   win->frame.size.x <= win->prev->frame.size.x
        && win->frame.size.y < win_meta->pad.size.y - winprev_maxy) {
      win->start.x = win->prev->start.x;                   // place window below previous window if it fits
      win->start.y = winprev_maxy + 1; }                   // vertically and is not wider than its predecessor
    else {
      struct Win * win_up = win->prev;
      struct Win * win_upup = win_up;
      uint16_t widthdiff;
      while (win_up != win_top) {
        win_upup = win_up->prev;
        while (1) {
          if (win_up->start.y != win_upup->start.y)
            break;
          win_upup = win_upup->prev; }
        winprev_maxy = win_upup->start.y + getmaxy(win_upup->frame.curses_win);
        widthdiff = (win_upup->start.x + win_upup->frame.size.x) - (win_up->start.x + win_up->frame.size.x);
        if (win->frame.size.y < win_meta->pad.size.y - winprev_maxy && win->frame.size.x < widthdiff) {
          win->start.x = win_up->start.x + win_up->frame.size.x + 1 ; // else try to open new sub column under
          win->start.y = winprev_maxy + 1;                     // last window below which enough space remains
          break; }
        win_up = win_upup; } } } }

static void update_windows (struct WinMeta * win_meta, struct Win * win) {
// Update geometry of win and its next of kin. Destroy (if visible), (re-)build window. If need, resize pad.
  if (0 != win->frame.curses_win)
    destroy_window (win);
  place_window(win_meta, win);
  refit_pad(win_meta);
  win->frame.curses_win = subpad(win_meta->pad.curses_win, win->frame.size.y, win->frame.size.x, win->start.y,
                                 win->start.x);
  if (0 != win->next)
    update_windows (win_meta, win->next); }

static void destroy_window (struct Win * win) {
// Delete window.
  delwin(win->frame.curses_win);
  win->frame.curses_win = 0; }

static void draw_window_borders (struct Win * win, char active) {
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

static void draw_windows_borders (struct Win * win, struct Win * win_active, struct Corners * corners,
                                  uint16_t ccount) {
// Craw draw_window_borders() for all windows in chain from win on. Save current window's border corners.
  char active = 0;
  if (win == win_active)
    active = 1;
  draw_window_borders(win, active);
  corners[ccount].tl.y = win->start.y - 1;
  corners[ccount].tl.x = win->start.x - 1;
  corners[ccount].tr.y = win->start.y - 1;
  corners[ccount].tr.x = win->start.x + win->frame.size.x;
  corners[ccount].bl.y = win->start.y + win->frame.size.y;
  corners[ccount].bl.x = win->start.x - 1;
  corners[ccount].br.y = win->start.y + win->frame.size.y;
  corners[ccount].br.x = win->start.x + win->frame.size.x;
  if (0 != win->next) {
    draw_windows_borders (win->next, win_active, corners, ccount + 1); } }

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
  char * scrolldsc = malloc((4 * sizeof(char)) + strlen(more) + strlen(unit) + 10); // 10: strlen for uint32_t
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

extern void draw_all_windows (struct WinMeta * win_meta) {
// Draw pad with all windows and their borders, plus scrolling hints.
  erase();
  wnoutrefresh(win_meta->screen);
  werase(win_meta->pad.curses_win);
  if (win_meta->chain_start) {
    uint16_t n_wins = 1;
    struct Win * win_p = win_meta->chain_start;
    while (0 != win_p->next) {
      win_p = win_p->next;
      n_wins++; }
    struct Corners * all_corners = malloc(sizeof(struct Corners) * n_wins);
    draw_windows (win_meta->chain_start);
    draw_windows_borders (win_meta->chain_start, win_meta->active, all_corners, 0);
    uint16_t i;
    for (i = 0; i < n_wins; i++) {
      mvwaddch(win_meta->pad.curses_win, all_corners[i].tl.y, all_corners[i].tl.x, '+');
      mvwaddch(win_meta->pad.curses_win, all_corners[i].tr.y, all_corners[i].tr.x, '+');
      mvwaddch(win_meta->pad.curses_win, all_corners[i].bl.y, all_corners[i].bl.x, '+');
      mvwaddch(win_meta->pad.curses_win, all_corners[i].br.y, all_corners[i].br.x, '+'); }
    free(all_corners);
    uint16_t y;
    if (win_meta->pad_offset > 0)
      draw_scroll_hint(&win_meta->pad, win_meta->pad_offset, win_meta->pad_offset + 1, '<');
    if (win_meta->pad_offset + win_meta->pad.size.x < getmaxx(win_meta->pad.curses_win) - 1)
      for (y = 0; y < win_meta->pad.size.y; y++)
        draw_scroll_hint(&win_meta->pad, win_meta->pad_offset + win_meta->pad.size.x - 1,
                         getmaxx(win_meta->pad.curses_win) - (win_meta->pad_offset + win_meta->pad.size.x),
                                  '>');
    pnoutrefresh(win_meta->pad.curses_win, 0, win_meta->pad_offset, 0, 0, win_meta->pad.size.y,
                 win_meta->pad.size.x - 1); }
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
