struct World {
  struct KeyBinding * keybindings;
  struct KeysWinData * keyswindata;
  int turn;
  char * log;
  struct Map * map;
  struct Monster * monster;
  struct Player * player; };

struct Map {
  int width;
  int height;
  int offset_x;
  int offset_y;
  char * cells; };

struct Player {
  int y;
  int x; };

struct Monster {
  int y;
  int x; };

void toggle_window (struct WinMeta *, struct Win *);
void growshrink_active_window (struct WinMeta *, char);
struct Map init_map ();
void map_scroll (struct Map *, char);
void next_turn (struct World *);
void update_log (struct World *, char *);
char is_passable (struct World *, int, int);
void move_player (struct World *, char);
void player_wait(struct World *);
