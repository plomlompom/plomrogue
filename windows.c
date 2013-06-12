#include <stdlib.h>
#include <stdint.h>
#include <ncurses.h>
#include <string.h>
#include "windows.h"

struct yx {
  uint16_t y;
  uint16_t x; };

struct Corners {
  struct yx tl;
  struct yx tr;
  struct yx bl;
  struct yx br; };

static void refit_pad (struct WinMeta *);
static struct yx place_window (struct WinMeta *, struct Win *);
static void update_windows (struct WinMeta *, struct Win *);
static void destroy_window (struct Win *);
static void draw_windows_borders (struct Win *, struct Win *, struct Corners *, uint16_t);
static void draw_window_borders (struct Win *, char);
static void draw_windows (struct Win *);
static void draw_vertical_scroll_hint (struct WinMeta *, uint16_t, uint32_t, char);

extern struct WinMeta init_win_meta (WINDOW * screen) {
// Create and populate WinMeta struct with sane default values.
  struct WinMeta win_meta;
  win_meta.screen = screen;
  win_meta.height = getmaxy(screen);
  win_meta.width = getmaxx(screen);
  win_meta.chain_start = 0;
  win_meta.chain_end = 0;
  win_meta.pad_offset = 0;
  win_meta.pad = newpad(win_meta.height, 1);
  win_meta.active = 0;
  return win_meta; }

extern struct Win init_window (struct WinMeta * win_meta, char * title, void * data, void * func) {
// Create and populate Win struct with sane default values.
  struct Win win;
  win.prev = 0;
  win.next = 0;
  win.curses = 0;
  win.title = title;
  win.width = 20;
  win.height = win_meta->height - 1;
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
    if (win_p->startx + win_p->width > lastwincol + 1)
      lastwincol = win_p->startx + win_p->width - 1;
    win_p = win_p->next; }
  if (getmaxx(win_meta->pad) != lastwincol)
    wresize(win_meta->pad, getmaxy(win_meta->pad), lastwincol + 2); }

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

static struct yx place_window (struct WinMeta * win_meta, struct Win * win) {
// Based on position and sizes of previous window, find fitting place for current window.
  struct yx start;
  start.x = 0;                                     // if window is first in chain, place it on top-left corner
  start.y = 1;
  if (0 != win->prev) {
    struct Win * win_top = win->prev;
    while (win_top->starty != 1)
      win_top = win_top->prev;                                   // else, default to placing window in new top
    start.x = win_top->startx + win_top->width + 1;              // column to the right of the last one
    uint16_t winprev_maxy = win->prev->starty + getmaxy(win->prev->curses);
    if (win->width <= win->prev->width && win->height < win_meta->height - winprev_maxy) {
      start.x = win->prev->startx;                         // place window below previous window if it fits
      start.y = winprev_maxy + 1; }                        // vertically and is not wider than its predecessor
    else {
      struct Win * win_up = win->prev;
      struct Win * win_upup = win_up;
      uint16_t widthdiff;
      while (win_up != win_top) {
        win_upup = win_up->prev;
        while (1) {
          if (win_up->starty != win_upup->starty)
            break;
          win_upup = win_upup->prev; }
        winprev_maxy = win_upup->starty + getmaxy(win_upup->curses);
        widthdiff = (win_upup->startx + win_upup->width) - (win_up->startx + win_up->width);
        if (win->height < win_meta->height - winprev_maxy && win->width < widthdiff) {
          start.x = win_up->startx + win_up->width + 1;          // else try to open new sub column under last
          start.y = winprev_maxy + 1;                            // window below which enough space remains
          break; }
        win_up = win_upup; } } }
  return start; }

static void update_windows (struct WinMeta * win_meta, struct Win * win) {
// Update geometry of win and its next of kin. Destroy (if visible), (re-)build window. If need, resize pad.
  if (0 != win->curses)
    destroy_window (win);
  struct yx startyx = place_window(win_meta, win);
  win->startx = startyx.x;
  win->starty = startyx.y;
  refit_pad(win_meta);
  win->curses = subpad(win_meta->pad, win->height, win->width, startyx.y, startyx.x);
  if (0 != win->next)
    update_windows (win_meta, win->next); }

static void destroy_window (struct Win * win) {
// Delete window.
  delwin(win->curses);
  win->curses = 0; }

static void draw_window_borders (struct Win * win, char active) {
// Draw borders of window win, including title. Decorate in a special way if window is marked as active.
  uint16_t y, x;
  for (y = win->starty; y <= win->starty + win->height; y++) {
    mvwaddch(wgetparent(win->curses), y, win->startx - 1, '|');
    mvwaddch(wgetparent(win->curses), y, win->startx + win->width, '|'); }
  for (x = win->startx; x <= win->startx + win->width; x++) {
    mvwaddch(wgetparent(win->curses), win->starty - 1, x, '-');
    mvwaddch(wgetparent(win->curses), win->starty + win->height, x, '-'); }
  char min_title_length_visible = 3;        // 1 char minimal, plus 2 chars for decoration left/right of title
  if (win->width >= min_title_length_visible) {
    uint16_t title_offset = 0;
    if (win->width > strlen(win->title) + 2)
      title_offset = (win->width - (strlen(win->title) + 2)) / 2;                     // + 2 is for decoration
    uint16_t length_visible = strnlen(win->title, win->width - 2);
    char title[length_visible + 3];
    char decoration = ' ';
    if (1 == active)
      decoration = '$';
    memcpy(title + 1, win->title, length_visible);
    title[0] = title[length_visible + 1] = decoration;
    title[length_visible + 2] = '\0';
    mvwaddstr(wgetparent(win->curses), win->starty - 1, win->startx + title_offset, title); } }

static void draw_windows_borders (struct Win * win, struct Win * win_active, struct Corners * corners, uint16_t ccount) {
// Craw draw_window_borders() for all windows in chain from win on. Save current window's border corners.
  char active = 0;
  if (win == win_active)
    active = 1;
  draw_window_borders(win, active);
  corners[ccount].tl.y = win->starty - 1;
  corners[ccount].tl.x = win->startx - 1;
  corners[ccount].tr.y = win->starty - 1;
  corners[ccount].tr.x = win->startx + win->width;
  corners[ccount].bl.y = win->starty + win->height;
  corners[ccount].bl.x = win->startx - 1;
  corners[ccount].br.y = win->starty + win->height;
  corners[ccount].br.x = win->startx + win->width;
  if (0 != win->next) {
    draw_windows_borders (win->next, win_active, corners, ccount + 1); } }

static void draw_windows (struct Win * win) {
// Draw contents of all windows in window chain from win on.
  win->draw(win);
  if (0 != win->next) {
    draw_windows (win->next); } }

static void draw_vertical_scroll_hint (struct WinMeta * win_meta, uint16_t x, uint32_t more_cols, char dir) {
// Draw scroll hint line in win at col x of pad display, announce more_cols more columns in direction dir.
  uint16_t y, offset;
  char phrase[] = "more columns";
  char * scrolldesc = malloc((3 * sizeof(char)) + strlen(phrase) + 10); // 10 = max chars for uint32_t string
  sprintf(scrolldesc, " %d %s ", more_cols, phrase);
  offset = 1;
  if (win_meta->height > (strlen(scrolldesc) + 1))
    offset = (win_meta->height - strlen(scrolldesc)) / 2;
  for (y = 0; y < win_meta->height; y++)
    if (y >= offset && y < strlen(scrolldesc) + offset)
      mvwaddch(win_meta->pad, y, x, scrolldesc[y - offset] | A_REVERSE);
    else
      mvwaddch(win_meta->pad, y, x, dir | A_REVERSE);
  free(scrolldesc); }

extern void draw_all_windows (struct WinMeta * win_meta) {
// Draw pad with all windows and their borders, plus scrolling hints.
  erase();
  wnoutrefresh(win_meta->screen);
  werase(win_meta->pad);
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
      mvwaddch(win_meta->pad, all_corners[i].tl.y, all_corners[i].tl.x, '+');
      mvwaddch(win_meta->pad, all_corners[i].tr.y, all_corners[i].tr.x, '+');
      mvwaddch(win_meta->pad, all_corners[i].bl.y, all_corners[i].bl.x, '+');
      mvwaddch(win_meta->pad, all_corners[i].br.y, all_corners[i].br.x, '+'); }
    free(all_corners);
    uint16_t y;
    if (win_meta->pad_offset > 0)
      draw_vertical_scroll_hint(win_meta, win_meta->pad_offset, win_meta->pad_offset + 1, '<');
    if (win_meta->pad_offset + win_meta->width < getmaxx(win_meta->pad) - 1)
      for (y = 0; y < win_meta->height; y++)
        draw_vertical_scroll_hint(win_meta, win_meta->pad_offset + win_meta->width - 1,
                                  getmaxx(win_meta->pad) - (win_meta->pad_offset + win_meta->width), '>');
    pnoutrefresh(win_meta->pad, 0, win_meta->pad_offset, 0, 0, win_meta->height, win_meta->width - 1); }
  doupdate(); }

extern void resize_active_window (struct WinMeta * win_meta, uint16_t height, uint16_t width) {
// Grow or shrink currently active window. Correct its geometry and that of its followers.
  if (0 != win_meta->active && width > 0 && height > 0 && height < win_meta->height) {
    win_meta->active->height = height;
    win_meta->active->width  = width;
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
  if (new_offset >= 0 && new_offset + win_meta->width < getmaxx(win_meta->pad))
    win_meta->pad_offset = new_offset; }
