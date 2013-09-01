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
struct World;



/* Puts an identifier on each window, interfaces to its config file data. */
struct WinConf
{
    char id;
    struct Win * win;
    char * title;
    int16_t height;
    int16_t width;
    void (* draw) (struct Win *);
};



/* Create/initialize (from config files)/free Winconf / Win structs. */
extern void create_winconfs(struct World * world);
extern void init_winconfs(struct World * world);
extern void free_winconfs(struct World * world);
extern void init_wins(struct World * world);
extern void free_wins(struct World * world);



/* Reload windows in order and sizes defined in win config. */
extern void reload_win_config(struct World * world);



/* Get WinConf identified by winconf->id == "id", or that winconf's ->win. */
extern struct WinConf * get_winconf_by_id(struct World * world, char id);
extern struct Win * get_win_by_id(struct World * world, char id);



/* Toggle windows in world->wins in the order desribed by the first line of
 * config/windows/toggled_win_order, wherein each character may correspond to
 * one hardcoded window.
 */
extern void sorted_wintoggle(struct World * world);



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
