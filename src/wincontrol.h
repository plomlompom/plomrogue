/* wincontrol.h
 *
 * Routines that build on top of the windows library to provide a simple window
 * management API to the game.
 */

#ifndef WINCONTROL_H
#define WINCONTROL_H



#include <stdint.h> /* for uint8_t */
struct Win;
struct WinMeta;



/* Toggle display of a window "win".
 *
 * Return 0 on success, 1 on (ncurses pad/window memory allocation) error.
 */
extern uint8_t toggle_window(struct WinMeta * win_meta, struct Win * win);



/* Try to scroll virtual screen left ("dir" = "-") or right ("dir" = "+"),
 * subject to the limitations provided by the window manager via
 * reset_pad_offset().
 */
extern void scroll_pad(struct WinMeta * win_meta, char dir);



/* Try to grow or shrink the active window horizontally ("change" = "*"/"_") or
 * vertically ("change = "+"/"-") by one cell size, subject to the limitations
 * provided by the window manager via resize_active_win().
 *
 * Return 0 on success, 1 on (ncurses pad/window memory allocation) error.
 */
extern uint8_t growshrink_active_window(struct WinMeta * win_meta, char change);



#endif
