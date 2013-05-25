struct KeyBinding {
  char * name;
  uint16_t key; };

struct KeysWinData {
  uint16_t max;
  char edit;
  uint16_t select; };

void init_keybindings(struct World *);
void save_keybindings(struct World *);
uint16_t get_action_key (struct KeyBinding *, char *);
char * get_keyname(uint16_t);
void keyswin_mod_key (struct World *, struct WinMeta *);
void keyswin_move_selection (struct World *, char);
