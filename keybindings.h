struct KeyBinding {
  char * name;
  int key; };

struct KeysWinData {
  int max;
  char edit;
  int select; };

void init_keybindings(struct World *);
void save_keybindings(struct World *);
int get_action_key (struct KeyBinding *, char *);
char * get_keyname(int);
void keyswin_mod_key (struct World *, struct WinMeta *);
void keyswin_move_selection (struct World *, char);
