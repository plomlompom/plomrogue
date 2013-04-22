#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include "windows.h"

struct WinMeta init_win_meta (WINDOW * screen) {
// Create and populate WinMeta struct with sane default values.
  struct WinMeta win_meta;
  win_meta.height = screen->_maxy + 1;
  win_meta.chain_start = 0;
  win_meta.chain_end = 0;
  return win_meta; }

struct Win init_window (struct WinMeta * win_meta, char * title) {
// Create and populate Win struct with sane default values.
  struct Win win;
  win.prev = 0;
  win.next = 0;
  win.curses_win = 0;
  win.title = title;
  win.width = 20;
  win.height = win_meta->height;
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
  update_windows(win_meta, win);
  draw_all_windows(win_meta); }

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
  win->next = 0;
  if (0 != win_meta->chain_start)
    draw_all_windows(win_meta); }

void place_window (struct WinMeta * win_meta, struct Win * win) {
// Based on position and sizes of previous window, find fitting place for current window.
  win->start_x = 0; // if window is first in chain, place it on top-left corner
  win->start_y = 0;
  if (0 != win->prev) {
    win->start_x = win->prev->start_x + win->prev->width; // next best default: open new window column with it
    if (win->prev->height < win_meta->height) { // ... unless the previous window does not fill a whole column
      struct Win * last_ceiling;
      last_ceiling = win->prev;
      while (last_ceiling->start_y != 0                               // determine last window serving as a
             && (last_ceiling->prev->start_y == last_ceiling->start_y // ceiling to other windows or filling
                 || last_ceiling->prev->width > last_ceiling->width)) // the whole last column's width
         last_ceiling = last_ceiling->prev;
      if (win->prev == last_ceiling) {
        if (win->width <= win->prev->width
            && win->prev->start_y + win->prev->height + win->height <= win_meta->height) {
          win->start_x = win->prev->start_x;                         // if prev window is last ceiling, try to
          win->start_y = win->prev->start_y + win->prev->height; } } // fit window below it; else: use default
      else {
        int remaining_width = last_ceiling->width; // calculate free width remaining in last row of last
        struct Win * win_p = last_ceiling->next;   // window column
        while (win != win_p) {
          remaining_width = remaining_width - win_p->width;
          win_p = win_p->next; }
        if (win->width <= remaining_width && win->height <= win->prev->height) { // if enough space left in
          win->start_y = win->prev->start_y;                                     // last column, place window
          win->start_x = win->prev->start_x + win->prev->width; }                // here
        else if (win->width <= last_ceiling->width
                 && win->height + win->prev->height + win->prev->start_y <= win_meta->height ) {
          win->start_y = last_ceiling->next->start_y + last_ceiling->next->height; // else, try to put it
          win->start_x = last_ceiling->start_x; }                                  // below
        else                                                                  // else, put it next to max
          win->start_x = last_ceiling->width + last_ceiling->start_x; } } } } // width of the last last column

void update_windows (struct WinMeta * win_meta, struct Win * win) {
// Update geometry of win and its next of kin. Before, destroy window, if visible. After, (re-)build it.
  if (0 != win->curses_win)
    destroy_window (win);
  place_window(win_meta, win);
  if (win->start_y + win->height < win_meta->height) // dependent on window position,
    win->border_down = 1;                            // append space for borders to be drawn
  else
    win->border_down = 0;
  if (win->start_x > 0)
    win->border_left = 1;
  else
    win->border_left = 0;
  win->curses_win = newwin(win->height + win->border_down, win->width + win->border_left, win->start_y, win->start_x - win->border_left);
  if (0 != win->next)
    update_windows (win_meta, win->next); }

void destroy_window (struct Win * win) {
// Undraw and delete window.
  undraw_window (win->curses_win);
  delwin(win->curses_win);
  win->curses_win = 0; }

void draw_windows (struct WinMeta * win_meta, struct Win * win) {
// Draw all windows from the current one on.
  draw_window(win_meta, win);
  if (0 != win->next)
    draw_windows (win_meta, win->next); }

void draw_all_windows (struct WinMeta * win_meta) {
// Draw all windows from the chain start on.
  draw_windows (win_meta, win_meta->chain_start); }

void draw_window(struct WinMeta * win_meta, struct Win * win) {
// Draw win's content, including border and title (the latter dependent on space available for it).
  char ls = '|';
  char rs = '|';
  char ts = '-';
  char bs = '-';
  char tl = '-';
  char tr = '+';
  char bl = '|';
  char br = '|';
  if (1 == win->border_down) {
    bl = '+';
    br = '+'; }
  if (1 == win->border_left)
    tl = '+';
  wborder(win->curses_win, ls, rs, ts, bs, tl, tr, bl, br);
  char min_title_length_visible = 3; // 1 char minimal, plus 2 chars for decoration left/right of title
  if (win->width > min_title_length_visible) {
    int title_length = strlen(win->title);
    int title_offset = (((win->width) - (title_length + 2)) / 2) + win->border_left; // + 2 is for decoration
    if (title_offset < win->border_left)
      title_offset = win->border_left;
    int length_visible = strnlen(win->title, win->width - min_title_length_visible);
    char title[length_visible + 3];
    char decoration = ' ';
    if (win_meta->active == win)
      decoration = '$';
    memcpy(title + 1, win->title, length_visible);
    title[0] = title[length_visible + 1] = decoration;
    title[length_visible + 2] = '\0';
    mvwaddstr(win->curses_win, 0, title_offset, title); }
  if (win->height > 1 && win->width > 1) ;
    win->draw(win);
  wrefresh(win->curses_win); }

void undraw_window (WINDOW * win) {
// Fill entire window with whitespace.
  int y, x;
  for (y = 0; y <= win->_maxy; y++)
    for (x = 0; x <= win->_maxx; x++)
      mvwaddch(win, y, x, ' ');
  wrefresh(win); }

void resize_window (struct WinMeta * win_meta, char change) {
// Grow or shrink currently active window. Correct its geometry and that of its followers.
  if      (change == '-' && win_meta->active->height > 2)
      win_meta->active->height--;
  else if (change == '+' && win_meta->active->height < win_meta->height)
    win_meta->active->height++;
  else if (change == '_' && win_meta->active->width > 2)
      win_meta->active->width--;
  else if (change == '*')
    win_meta->active->width++;
  update_windows(win_meta, win_meta->chain_start);
  draw_all_windows(win_meta); }

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
      win_meta->active = win_meta->chain_end; }
  draw_all_windows(win_meta); }

void shift_window (struct WinMeta * win_meta, char dir) {
// Move active window forward/backward in window chain. If jumping beyond start/end, move to other chain end.
  if (win_meta->active != win_meta->chain_start || win_meta->active != win_meta->chain_end) {
    if ('f' == dir) {
      if (win_meta->active == win_meta->chain_end) { // move forward beyond chain end
        win_meta->active->prev->next = 0;
        win_meta->chain_end = win_meta->active->prev;
        win_meta->active->prev = 0;
        win_meta->active->next = win_meta->chain_start;
        win_meta->chain_start->prev = win_meta->active;
        win_meta->chain_start = win_meta->active; }
      else {                                        // move forward before chain end
        if (win_meta->chain_start != win_meta->active)
          win_meta->active->prev->next = win_meta->active->next;
        else
          win_meta->chain_start = win_meta->active->next;
        win_meta->active->next->prev = win_meta->active->prev;
        win_meta->active->prev = win_meta->active->next;
        win_meta->active->next = win_meta->active->next->next;
        win_meta->active->prev->next = win_meta->active;
        if (0 != win_meta->active->next)
          win_meta->active->next->prev = win_meta->active;
        else
          win_meta->chain_end = win_meta->active; } }
    else { // mirror of above, backwards
      if (win_meta->active == win_meta->chain_start) {
        win_meta->active->next->prev = 0;
        win_meta->chain_start = win_meta->active->next;
        win_meta->active->next = 0;
        win_meta->active->prev = win_meta->chain_end;
        win_meta->chain_end->next = win_meta->active;
        win_meta->chain_end = win_meta->active; }
      else {
        if (win_meta->chain_end != win_meta->active)
          win_meta->active->next->prev = win_meta->active->prev;
        else
          win_meta->chain_end = win_meta->active->prev;
        win_meta->active->prev->next = win_meta->active->next;
        win_meta->active->next = win_meta->active->prev;
        win_meta->active->prev = win_meta->active->prev->prev;
        win_meta->active->next->prev = win_meta->active;
        if (0 != win_meta->active->prev)
          win_meta->active->prev->next = win_meta->active;
        else
          win_meta->chain_start = win_meta->active; } }
  update_windows(win_meta, win_meta->chain_start);
  draw_all_windows(win_meta); } }
