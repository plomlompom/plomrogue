/* src/client/wincontrol.h
 *
 * This file is part of PlomRogue. PlomRogue is licensed under the GPL version 3
 * or any later version. For details on its copyright, license, and warranties,
 * see the file NOTICE in the root directory of the PlomRogue source package.
 *
 * Window manipulation functions directly called by user actions.
 */

#ifndef WINCONTROL_H
#define WINCONTROL_H



/* Toggle display of a window of "id". */
extern void toggle_window(char id);

/* Toggle "window configuration" view for active window. Sets sensible .center
 * values for each configuration view (for winconf_geometry: y=0, x=0; for
 * winconf_keys: x=0 (y is set by draw_winconf_keybindings()); stores default
 * view's .center in .target_center to return to it when toggling back.
 */
extern void toggle_winconfig();

/* Toggle active window's .target_(height/width)_type ("axis" = "y": height;
 * else: width). Don't toggle to .target_width_type of 1 (saving the width as a
 * diff to the .t_screen's width) if window's width is larger than .t_screen's
 * width, for such width is better saved directly with .target_width_type of 0.
 */
extern void toggle_win_size_type(char axis);

/* Toggle window's line break type. (0: line breaks only at newlines; 1:
 * linebreaks at newlines and when the text hits the right window border; 2:
 * linebreaks only when the text hits the right window border, newlines are
 * replaced by another string).
 */
extern void toggle_linebreak_type();

/* Grow or shrink active window horizontally ("change" = "*"/"_") or vertically
 * ("change" = "+"/"-") if the new size was at least 1x1, the height at least
 * one cell smaller than .v_screen's vertical hight (to provide space for the
 * title bar) and the width max. (2^16) - 1 cells. If a new window width would
 * surpass that of .t_screen, set active window's .target_width_type to 0.
 */
extern void resize_active_win(char c);

/* Move active window forwards ("dir" == "f") or backwards (any other "dir") in
 * window chain. Wrap around in the window chain if start / end of it is met.
 */
extern void shift_active_win(char dir);

/* Sroll .v_screen one cell to the left if "dir" is "-" and .v_screen_offset is
 * more than 1, or to the right if "dir" is "+" and .v_screen's right edge would
 * not move (further, if suspension of windows has moved it to the left already)
 * leftwards to .t_screen's right edge.
 */
extern void scroll_v_screen(char dir);

/* Cycle active window selection forwards ("dir" == "f") or backwards (any
 * other "dir"). Wrap around in the windows chain if start / end of it is met.
 */
extern void cycle_active_win(char dir);



#endif
