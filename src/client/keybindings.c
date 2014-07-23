/* src/client/keybindings.c */

#include "keybindings.h"
#include <ncurses.h> /* keycode defines, cbreak(), halfdelay(), getch() */
#include <stddef.h> /* NULL */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#include <stdio.h> /* FILE, sprintf() */
#include "../common/rexit.h" /* exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "windows.h" /* draw_all_wins() */
#include "world.h" /* global world */
struct Command;


/* Return pointer to global keybindings or to keybindings for wingeometry config
 * (c = "g") or winkeys config (c = "k") or active window's keybindings ("w").
 */
static struct KeyBindingDB * char_selected_kb_db(char c);

/* If "keycode_given" equals "keycode_match", copy "keyname_match" to "keyname"
 * and return 1; otherwise return 0.
 */
static uint8_t try_keycode(uint16_t keycode_given, char * keyname,
                           uint16_t keycode_match, char * keyname_match);



static struct KeyBindingDB * char_selected_kb_db(char c)
{
    struct KeyBindingDB * kbdb;
    kbdb = &world.kb_global;
    if      ('g' == c)
    {
        kbdb = &world.kb_wingeom;
    }
    else if ('k' == c)
    {
        kbdb = &world.kb_winkeys;
    }
    else if ('w' == c)
    {
        struct Win * w = get_win_by_id(world.winDB.active);
        kbdb = &w->kb;
    }
    return kbdb;
}



static uint8_t try_keycode(uint16_t keycode_given, char * keyname,
                           uint16_t keycode_match, char * keyname_match)
{
    if (keycode_given == keycode_match)
    {
        int test = sprintf(keyname, "%s", keyname_match);
        exit_trouble(test < 0, __func__, "sprintf");
        return 1;
    }
    return 0;
}



extern struct Command * get_command_to_keycode(struct KeyBindingDB * kbdb,
                                               uint16_t keycode)
{
    uint8_t n_kb;
    for (n_kb = 0; n_kb < kbdb->n_of_kbs; n_kb++)
    {
        if (keycode == kbdb->kbs[n_kb].keycode)
        {
            return kbdb->kbs[n_kb].command;
        }
    }
    return NULL;
}



extern char * get_keyname_to_keycode(uint16_t keycode)
{
    char * keyname = try_malloc(10, __func__);      /* max keyname length + 1 */
    if (32 < keycode && keycode < 127)
    {
        exit_trouble(sprintf(keyname, "%c", keycode) < 0, __func__, "sprintf");
    }
    else if (keycode >= KEY_F0 && keycode <= KEY_F(63))
    {
        uint16_t f = keycode - KEY_F0;
        exit_trouble(sprintf(keyname, "F%d", f) < 0, __func__, "sprintf");;
    }
    else if (   try_keycode(keycode, keyname, 9, "TAB")
             || try_keycode(keycode, keyname, 10, "RETURN")
             || try_keycode(keycode, keyname, 27, "ESCAPE")
             || try_keycode(keycode, keyname, 32, "SPACE")
             || try_keycode(keycode, keyname, KEY_UP, "UP")
             || try_keycode(keycode, keyname, KEY_DOWN, "DOWN")
             || try_keycode(keycode, keyname, KEY_LEFT, "LEFT")
             || try_keycode(keycode, keyname, KEY_RIGHT, "RIGHT")
             || try_keycode(keycode, keyname, KEY_HOME, "HOME")
             || try_keycode(keycode, keyname, KEY_BACKSPACE, "BACKSPACE")
             || try_keycode(keycode, keyname, KEY_DC, "DELETE")
             || try_keycode(keycode, keyname, KEY_IC, "INSERT")
             || try_keycode(keycode, keyname, KEY_NPAGE, "NEXT PAGE")
             || try_keycode(keycode, keyname, KEY_PPAGE, "PREV PAGE")
             || try_keycode(keycode, keyname, KEY_END, "END"))
    {
        ;
    }
    else
    {
        exit_trouble(sprintf(keyname, "(unknown)") < 0, __func__, "sprintf");
    }
    return keyname;
}



extern void mod_selected_keyb(char kb_c)
{
    struct KeyBindingDB * kbdb = char_selected_kb_db(kb_c);
    kbdb->edit = 1;
    draw_all_wins();
    cbreak();
    int keycode = getch();
    halfdelay(world.halfdelay);
    if (keycode < 1000)
    {
        kbdb->kbs[kbdb->select].keycode = keycode;
    }
    kbdb->edit = 0;
}



extern void move_keyb_selection(char kb_c, char dir)
{
    struct KeyBindingDB * kbdb = char_selected_kb_db(kb_c);
    if      ('u' == dir && kbdb->select > 0)
    {
        kbdb->select--;
    }
    else if ('d' == dir && kbdb->select < kbdb->n_of_kbs - 1)
    {
        kbdb->select++;
    }
}
