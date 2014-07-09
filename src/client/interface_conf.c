/* src/client/interface_conf.c */

#define _POSIX_C_SOURCE 200809L /* getopt(), optarg, strdup() */
#include "interface_conf.h"
#include <ncurses.h> /* delwin() */
#include <stddef.h> /* NULL, size_t */
#include <stdint.h> /* UINT8_MAX, uint8_t, uint32_t */
#include <stdlib.h> /* EXIT_SUCCESS, atoi(), exit(), free() */
#include <stdio.h> /* FILE, sprintf() */
#include <string.h> /* strchr(), strcmp(), strdup(), strlen() */
#include <unistd.h> /* optarg, getopt() */
#include "../common/parse_file.h" /* EDIT_STARTED, parse_file(),parse_flagval(),
                                   * token_from_line(), parsetest_singlechar(),
                                   * parse_and_reduce_to_readyflag(),
                                   * parsetest_defcontext(),parse_unknown_arg(),
                                   * parsetest_too_many_values(),
                                   * parse_id_uniq(), parse_init_entry()
                                   */
#include "../common/readwrite.h" /* atomic_write_start(), atomic_write_finish()
                                  * try_fwrite()
                                  */
#include "../common/rexit.h" /* exit_err(), exit_trouble() */
#include "../common/try_malloc.h" /* try_malloc() */
#include "array_append.h" /* array_append() */
#include "cleanup.h" /* set_cleanup_flag() */
#include "command_db.h" /* get_command() */
#include "keybindings.h" /* KeyBinding, KeyBindingDB, get_command_to_keycode()*/
#include "map.h" /* map_center() */
#include "wincontrol.h" /* toggle_window() */
#include "windows.h" /* Win, free_winDB(), make_v_screen_and_init_win_sizes() */
#include "world.h" /* global world */



/* Editing state flags set / checked in tokens_into_entries(). */
enum flag
{
    NAME_SET   = 0x02,
    WIDTH_SET  = 0x04,
    HEIGHT_SET = 0x08,
    BREAK_SET  = 0x10,
    FOCUS_SET  = 0x02,
    READY_WIN  = NAME_SET | WIDTH_SET | HEIGHT_SET | BREAK_SET,
    READY_ORD  = 0x00,
    READY_KBD  = 0x00
};



/* Used in load_interface_conf() to temporarily store designated values for
 * world.winDB.order and world.winDB.active. If those were set right away
 * without all windows having already been initialized, orderly exits on error
 * would be impossible, for windows are cleaned out by working toggling them off
 * piece by peice following these values in unload_interfac_conf().
 */
static char * tmp_order;
static char tmp_active;



/* Write into "file" keybindings in "kbdb" in config file format. */
static void write_keybindings(FILE * file, struct KeyBindingDB * kbdb);

/* Write into file "val" interpreted as pointing either to string (type = 's'),
 * int16 ('i') or char ('c'), prefixed with "prefix" and put into single quotes
 * if 0 != "quotes".
 */
static void write_def(FILE * file, char * prefix, uint8_t quotes, char * val,
                      char type);

/* Read in "token0" and "token1" as interface config definition lines, apply
 * definitions to WinDB, KeyBindingsDBs and tmp_order/tmp_active. If argument is
 * "KEY", get third token via token_from_line() for complete keybinding.
 */
static void tokens_into_entries(char * token0, char * token1);

/* If "win" points to non-NULL Win struct, append to it to WinDB.wins array and
 * add its ID to world.winDB.ids.
 */
static void write_if_win(struct Win ** win);

/* Start block setting data for window "token1" id if "token0" == "str_win".*/
static uint8_t start_win(char * token0, char * token1, char * str_win,
                         uint8_t * win_flags, struct Win ** win);

/* Start block setting window order and focus if "token0" = "str_ord".
 * "tmp_order" stores the designated world.winDB.order string (only later to
 * be realized in load_interface_conf() when all windows have been initialized),
 * as does "tmp_active" for the designated world.winDB.active char, defaulting
 * to the first char in the window order string if not otherwise specified.
 */
static uint8_t start_ord(char * token0, char * token1, char * str_ord,
                         uint8_t * ord_flags);

/* Start block setting non-window-specific keybindings if "token0" == "str_kbd".
 * "token1" must be "global", "wingeom" or "winkeys" to set KeyBindingDB to edit
 * ("kbdb") to world.kb_global, world.kb_wingeom or world.kb_winkeys.
 */
static uint8_t start_kbd(char * token0, char * token1, char * str_kbd,
                         uint8_t * kbd_flags, struct KeyBindingDB ** kbdb);

/* Helper to tokens_into_entries(), sets specific entry member data. "token0"
 * and the state of flags determine what entry member to edit; "win" and "kbdb"
 * are entries to write "token1" data into (as is the global "tmp_active"). If
 * "token0" is "str_key", a keybinding is defined and "token2" is read/must not
 * be NULL. In that case, the line read is checked against having a 4th token.
 */
static uint8_t set_members(char * token0, char * token1, uint8_t * win_flags,
                           uint8_t * ord_flags,uint8_t kbd_flags,char * str_key,
                           struct Win * win, struct KeyBindingDB * kbdb);

/* Add keybinding defined in "token1" as keycode and "token2" as command to
 * "kbdb" if "flags" are set to EDIT_STARTED.
 */
static void set_keybindings(char * token1, uint8_t flags,
                            struct KeyBindingDB * kbdb);



static void write_keybindings(FILE * file, struct KeyBindingDB * kbdb)
{
    char * f_name = "write_keybindings()";
    char * sep = " ";
    char * tok0 = "KEY";
    uint8_t i_kb;
    for (i_kb = 0; i_kb < kbdb->n_of_kbs; i_kb++)
    {
        size_t size = strlen(tok0) + strlen(sep) + 3 + strlen(sep)
                      + strlen(kbdb->kbs[i_kb].command->dsc_short) + 1 + 1;
        char * line = try_malloc(size, f_name);
        int test = snprintf(line, size, "%s%s%d%s%s\n",
                             tok0, sep, kbdb->kbs[i_kb].keycode, sep,
                             kbdb->kbs[i_kb].command->dsc_short);
        exit_trouble(test < 0, f_name, "snprintf()");
        try_fwrite(line, sizeof(char), strlen(line), file, f_name);
        free(line);
    }
}



static void write_def(FILE * file, char * prefix, uint8_t quotes, char * val,
                      char type)
{
    char * f_name = "write_def()";
    char * val_str = NULL;
    int test_val_str = 1;
    if      ('s' == type)
    {
        val_str = strdup(val);
    }
    if      ('i' == type)
    {
        size_t size_val_str = 6 + 1;
        val_str = try_malloc(size_val_str, f_name);
        test_val_str = snprintf(val_str, size_val_str, "%d", (int16_t) *val);
    }
    else if ('c' == type)
    {
        size_t size_val_str = 1 + 1;
        val_str = try_malloc(size_val_str, f_name);
        test_val_str = snprintf(val_str, size_val_str, "%c", * val);
    }
    exit_trouble(test_val_str < 0, f_name, "snprintf()");
    char * quote = quotes ? "'": "";
    char * affix = "\n";
    size_t size =   strlen(prefix) + strlen(val_str) + (2 * strlen(quote))
                  + strlen(affix) + 1;
    char * line = try_malloc(size, f_name);
    int test = snprintf(line, size, "%s%s%s%s%s",
                        prefix, quote, val_str, quote, affix);
    exit_trouble(test < 0, f_name, "snprintf()");
    free(val_str);
    try_fwrite(line, sizeof(char), strlen(line), file, f_name);
    free(line);
}



static void tokens_into_entries(char * token0, char * token1)
{
    char * str_win = "WINDOW";
    char * str_ord = "WIN_ORDER";
    char * str_kbd = "KEYBINDINGS";
    char * str_key = "KEY";
    static uint8_t win_flags = READY_WIN;
    static uint8_t ord_flags = READY_ORD;
    static uint8_t kbd_flags = READY_KBD;
    static struct Win * win = NULL;
    static struct KeyBindingDB * kbdb = NULL;
    if (!token0 || !strcmp(token0, str_win) || !strcmp(token0, str_ord)
                || !strcmp(token0, str_kbd))
    {
        parse_and_reduce_to_readyflag(&win_flags, READY_WIN);
        parse_and_reduce_to_readyflag(&ord_flags, READY_ORD);
        parse_and_reduce_to_readyflag(&kbd_flags, READY_KBD);
        write_if_win(&win);
    }
    if (token0)
    {
        if (strcmp(token0, str_key))
        {
            parsetest_too_many_values();
        }
        if (!(   start_win(token0, token1, str_win, &win_flags, &win)
              || start_ord(token0, token1, str_ord, &ord_flags)
              || start_kbd(token0, token1, str_kbd, &kbd_flags, &kbdb)
              || set_members(token0, token1, &win_flags, &ord_flags, kbd_flags,
                             str_key, win, kbdb)))
        {
           parse_unknown_arg();
        }
    }
}



static void write_if_win(struct Win ** win)
{
    char * f_name = "write_if_win()";
    if (*win)
    {
        (*win)->target_height_type = (0 >= (*win)->target_height);
        (*win)->target_width_type = (0 >= (*win)->target_width);;
        size_t old_ids_size = strlen(world.winDB.ids);
        size_t new_size = old_ids_size + 1 + 1;
        char * new_ids = try_malloc(new_size, f_name);
        int test = snprintf(new_ids,new_size,"%s%c",world.winDB.ids,(*win)->id);
        exit_trouble(test < 0, f_name, "snprintf()");
        free(world.winDB.ids);
        world.winDB.ids = new_ids;
        array_append(old_ids_size, sizeof(struct Win), *win,
                     (void **) &world.winDB.wins);
        free(*win);
        *win = NULL;
    }
}



static uint8_t start_win(char * token0, char * token1, char * str_win,
                         uint8_t * win_flags, struct Win ** win)
{
    if (strcmp(token0, str_win))
    {
        return 0;
    }
    *win = (struct Win *) parse_init_entry(win_flags, sizeof(struct Win));
    parsetest_singlechar(token1);
    parse_id_uniq(world.winDB.ids && (NULL!=strchr(world.winDB.ids,token1[0])));
    (*win)->id = token1[0];
    return 1;
}



static uint8_t start_ord(char * token0, char * token1, char * str_ord,
                         uint8_t * ord_flags)
{
    if (strcmp(token0, str_ord))
    {
        return 0;
    }
    *ord_flags = EDIT_STARTED;
    uint32_t i = 0;
    for (; i < strlen(token1); i++)
    {
        err_line(!strchr(world.winDB.legal_ids, token1[i]), "Illegal ID(s).");
    }
    free(tmp_order);
    tmp_order = strdup(token1);
    if (0 < strlen(tmp_order))
    {
        tmp_active = tmp_order[0];
    }
    return 1;
}



static uint8_t start_kbd(char * token0, char * token1, char * str_kbd,
                         uint8_t * kbd_flags, struct KeyBindingDB ** kbdb)
{
    if (strcmp(token0, str_kbd))
    {
        return 0;
    }
    *kbd_flags = EDIT_STARTED;
    if      (!strcmp(token1, "global"))
    {
        *kbdb = &world.kb_global;
    }
    else if (!strcmp(token1, "wingeom"))
    {
        *kbdb = &world.kb_wingeom;
    }
    else if (!strcmp(token1, "winkeys"))
    {
        *kbdb = &world.kb_winkeys;
    }
    else
    {
        err_line(1, "Value must be 'global', 'wingeom' or 'winkeys'.");
    }
    return 1;
}



static uint8_t set_members(char * token0, char * token1, uint8_t * win_flags,
                           uint8_t * ord_flags,uint8_t kbd_flags,char * str_key,
                           struct Win * win, struct KeyBindingDB * kbdb)
{
    if (   parse_flagval(token0, token1, "NAME", win_flags,
                         NAME_SET, 's', (char *) &win->title)
        || parse_flagval(token0, token1, "WIDTH", win_flags,
                         WIDTH_SET, 'i', (char *) &win->target_width)
        || parse_flagval(token0, token1, "HEIGHT", win_flags,
                         HEIGHT_SET, 'i', (char *) &win->target_height));
    else if (parse_flagval(token0, token1, "BREAK", win_flags,
                           BREAK_SET, '8', (char *) &win->linebreak))
    {
        err_line(2 < win->linebreak, "Value must be 0, 1 or 2.");
    }
    else if (parse_flagval(token0, token1, "WIN_FOCUS", ord_flags,
                           FOCUS_SET, 'c', &tmp_active))
    {
        char * err_null = "Value not empty as it should be.";
        char * err_outside = "ID not found in WIN_ORDER ID series.";
        err_line(!strlen(tmp_order) && tmp_active, err_null);
        err_line(!strchr(tmp_order, tmp_active), err_outside);
    }
    else if (!strcmp(token0, str_key))
    {
        if (*win_flags & EDIT_STARTED)
        {
            set_keybindings(token1, *win_flags, &win->kb);
        }
        else
        {
            set_keybindings(token1, kbd_flags, kbdb);
        }
    }
    else
    {
        return 0;
    }
    return 1;
}



static void set_keybindings(char * token1, uint8_t flags,
                            struct KeyBindingDB * kbdb)
{
    char * token2 = token_from_line(NULL);
    err_line(!token2, "No binding to key given.");
    parsetest_too_many_values();
    char * err_code = "Invalid keycode. Must be >= 0 and < 1000.";
    parsetest_defcontext(flags);
    char * err_many = "No more than 255 keybindings allowed in one section.";
    err_line(UINT8_MAX == kbdb->n_of_kbs, err_many);
    struct KeyBinding kb;
    uint8_t test = strlen(token1);
    err_line(!test, err_code);
    uint8_t i;
    for (i = 0; '\0' != token1[i]; i++)
    {
        test= i > 2 || '0' > token1[i] || '9' < token1[i];
        err_line(test, err_code);
    }
    kb.keycode = atoi(token1);
    char * err_uniq = "Binding to key already defined.";
    err_line(NULL != get_command_to_keycode(kbdb, kb.keycode), err_uniq);
    kb.command = get_command(token2);
    err_line(!(kb.command), "No such command in command DB.");
    array_append(kbdb->n_of_kbs, sizeof(struct KeyBinding), (void *) &kb,
                 (void **) kbdb);
    kbdb->n_of_kbs++;
}



extern void obey_argv(int argc, char * argv[])
{
    int opt;
    while (-1 != (opt = getopt(argc, argv, "i:")))
    {
        if      ('i' == opt)
        {
            world.path_interface = optarg;
        }
        else
        {
            exit(EXIT_FAILURE);
        }
    }
}



extern void save_interface_conf()
{
    char * path_tmp;
    FILE * file = atomic_write_start(world.path_interface, &path_tmp);
    char * str_keybs = "\nKEYBINDINGS ";
    write_def(file, str_keybs, 1, "global", 's');
    write_keybindings(file, &world.kb_global);
    write_def(file, str_keybs, 1, "wingeom", 's');
    write_keybindings(file, &world.kb_wingeom);
    write_def(file, str_keybs, 1, "winkeys", 's');
    write_keybindings(file, &world.kb_winkeys);
    write_def(file, "\nWIN_ORDER ", 1, world.winDB.order, 's');
    if (world.winDB.active)
    {
        write_def(file, "WIN_FOCUS ", 1, &world.winDB.active, 'c');
    }
    uint8_t i;
    for (i = 0; i < strlen(world.winDB.ids); i++)
    {
        write_def(file, "\nWINDOW ", 0, &world.winDB.ids[i], 'c');
        struct Win * win = get_win_by_id(world.winDB.ids[i]);
        write_def(file, "NAME ", 1, win->title, 's');
        write_def(file, "BREAK ", 0, (char *) &win->linebreak, 'i');
        write_def(file, "WIDTH ", 0, (char *) &win->target_width, 'i');
        write_def(file, "HEIGHT ", 0, (char *) &win->target_height, 'i');
        write_keybindings(file, &win->kb);
    }
    atomic_write_finish(file, world.path_interface, path_tmp);
}



extern void load_interface_conf()
{
    char * f_name = "load_interface_conf()";
    world.winDB.ids    = try_malloc(1, f_name);
    world.winDB.ids[0] = '\0';
    world.winDB.wins = NULL;
    tmp_order    = try_malloc(1, f_name);
    tmp_order[0] = '\0';
    tmp_active   = '\0';
    parse_file(world.path_interface, tokens_into_entries);
    char * err = "Not all expected windows defined in config file.";
    exit_err(strlen(world.winDB.legal_ids) != strlen(world.winDB.ids), err);
    make_v_screen_and_init_win_sizes();
    world.winDB.order = try_malloc(1, f_name);
    world.winDB.order[0] = '\0';
    uint8_t i;
    for (i = 0; i < strlen(tmp_order); toggle_window(tmp_order[i]), i++);
    world.winDB.active = tmp_active;
    free(tmp_order);
    set_cleanup_flag(CLEANUP_INTERFACE);
}



extern void unload_interface_conf()
{
    free(world.kb_global.kbs);
    world.kb_global.kbs = NULL;
    world.kb_global.n_of_kbs = 0;
    free(world.kb_wingeom.kbs);
    world.kb_wingeom.kbs = NULL;
    world.kb_wingeom.n_of_kbs = 0;
    free(world.kb_winkeys.kbs);
    world.kb_winkeys.kbs = NULL;
    world.kb_winkeys.n_of_kbs = 0;
    while ('\0' != world.winDB.active)
    {
        toggle_window(world.winDB.active);
    }
    free_winDB();
    delwin(world.winDB.v_screen);
}



extern void reload_interface_conf()
{
    unload_interface_conf();
    load_interface_conf();
    map_center();
    world.winDB.v_screen_offset = 0;
}
