/* wincontrol.h
 *
 * Routines that build on top of the windows library to provide a simple window
 * management API to the game.
 */

#ifndef WINCONTROL_H
#define WINCONTROL_H

#include <stdint.h> /* for uint8_t, int16_t */
#include "keybindings.h" /* for KeyBiData struct */
#include "yx_uint16.h" /* for yx_uint16 struct */
struct Win;
struct WinMeta;



/* Stores a window's configuration (like geometry, keybindings) and a pointer to
 * the respective Win struct itself.
 */
struct WinConf
{
    char id; /* unique identifier of WinConf, abused as ID for ->win and  */
             /* equivalent to the char following its "Win_" conffile name */
    struct Win * win; /* window configured by this WinConf */
    char * title; /* designated title as passed to init_win() */
    int16_t height; /* designated height as interpreted by init_win()*/
    int16_t width; /* designated width as interpreted by init_win() */
    char draw; /* identifier of designated Win->draw; to be returned to */
               /* after toggling window configuration view */
    struct yx_uint16 center; /* designated center for Win->draw view; to be */
                             /* returned to after toggling winconf view */
    uint8_t view; /* 0: use ->draw as Win->draw; 1, 2: use draw_winconf()_* */
    uint8_t height_type; /* both: 0: interpret ->height/->width as size in   */
    uint8_t width_type;  /* positive cells; 1: as negative diff to max width */
    struct KeyBiData kb; /* the window's specific keybindings */
};



/* Get WinConf fathering "win"; get Win of WinConf of "id". */
extern struct WinConf * get_winconf_by_win(struct Win * win);
extern struct Win * get_win_by_id(char id);



/* Create/initialize (from config files)/free Winconf structs. */
extern void init_winconfs();
extern void free_winconfs();
extern void init_wins();



/* Toggle windows in world.wins in the order desribed by the first line of
 * config/windows/toggle_order_and_active, wherein each character should
 * correspond to one window whose ID is found in world.winconf_ids. Unknown
 * characters are silently ignored. The first character of the second line of
 * the file is also read to determine which window to focus as active (but only
 * if it fits the ID of a window thus toggled visible).
 */
extern void sorted_wintoggle_and_activate();



/* Save all window's configurations to their configuration files. */
extern void save_win_configs();



/* Toggle "window configuration" view for "win". This also sets sensible values
 * for win->center for the various configuration views (y=0, x=0 for
 * winconf_geometry and x= for winconf_keys).
 */
extern void toggle_winconfig(struct Win * win);



/* Toggle interpretation type for Win's width/height of Win in WinConf. Width
 * only toggles to 1 if terminal window is at least as wide as WinConf->width.
 */
extern void toggle_win_height_type(struct Win * win);
extern void toggle_win_width_type(struct Win * win);



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
 * Forces WinConf->width_type = 0 if new width surpasses that of terminal win.
 *
 * Return 0 on success, 1 on (ncurses pad/window memory allocation) error.
 */
extern uint8_t growshrink_active_window(char change);



#endif
