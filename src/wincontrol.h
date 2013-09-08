/* wincontrol.h
 *
 * Routines that build on top of the windows library to provide a simple window
 * management API to the game.
 */

#ifndef WINCONTROL_H
#define WINCONTROL_H



#include <stdint.h> /* for uint8_t, int16_t */
struct Win;
struct WinMeta;
struct World;



/* Stores designated configuration of a window pointed to in it, and data used
 * to manipulate said window in the "window configuration" view of it.
 */
struct WinConf
{
    char id; /* unique identifier of WinConf, abused as ID for ->win and  */
             /* equivalent to the char following its "Win_" conffile name */
    struct Win * win; /* window configured by this WinConf */
    char * title; /* designated title as passed to init_win() */
    int16_t height; /* designated height as interpreted by init_win()*/
    int16_t width; /* designated width as interpreted by init_win() */
    char draw; /* identifier of designated Win->_draw; to be returned to */
               /* after toggling window configuration view */
    uint8_t view; /* 0: use ->draw as Win->_draw; 1: use draw_winconf()*/
    uint8_t height_type; /* both: 0: interpret ->height/->width as size in   */
    uint8_t width_type;  /* positive cells; 1: as negative diff to max width */
};



/* Get WinConf fathering "win"; get Win of WinConf of "id". */
extern struct WinConf * get_winconf_by_win(struct World * world,
                                           struct Win * win);
extern struct Win * get_win_by_id(struct World * world, char id);



/* Create/initialize (from config files)/free Winconf / Win structs. */
extern void init_winconfs(struct World * world);
extern void free_winconfs(struct World * world);
extern void init_wins(struct World * world);
extern void free_wins(struct World * world);



/* Toggle windows in world->wins in the order desribed by the first line of
 * config/windows/toggled_win_order, wherein each character should correspond to
 * one window the ID of which is found in world->winconf_ids. Unknown characters
 * are silently ignored.
 */
extern void sorted_wintoggle(struct World * world);

/* Reload windows in order and sizes defined in win config. */
extern void reload_win_config(struct World * world);

/* Save all window's configurations to their configuration files. */
extern void save_win_configs(struct World * world);



/* Toggle "window configuration" view for "win". */
extern void toggle_winconfig(struct World * world, struct Win * win);

/* Toggle interpretation type for Win's width/height of Win in WinConf. Width
 * only toggles to 1 if terminal window is at least as wide as WinConf->width.
 */
extern void toggle_win_height_type(struct World * world, struct Win * win);
extern void toggle_win_width_type(struct World * world, struct Win * win);



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
extern uint8_t growshrink_active_window(struct World * world, char change);



#endif
