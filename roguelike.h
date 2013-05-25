struct World {
  struct KeyBinding * keybindings;
  struct KeysWinData * keyswindata;
  uint16_t turn;
  char * log;
  struct Map * map;
  struct Monster * monster;
  struct Player * player; };

struct Map {
  uint16_t width;
  uint16_t height;
  uint16_t offset_x;
  uint16_t offset_y;
  char * cells; };

struct Player {
  uint16_t y;
  uint16_t x; };

struct Monster {
  uint16_t y;
  uint16_t x; };

void toggle_window (struct WinMeta *, struct Win *);
void growshrink_active_window (struct WinMeta *, char);
struct Map init_map ();
void map_scroll (struct Map *, char);
void next_turn (struct World *);
void update_log (struct World *, char *);
char is_passable (struct World *, uint16_t, uint16_t);
void move_player (struct World *, char);
void player_wait(struct World *);
