struct World {
  char interactive;
  struct KeyBinding * keybindings;
  struct KeysWinData * keyswindata;
  uint32_t seed;
  uint32_t turn;
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
  struct yx_uint16 pos; };

struct Monster {
  struct Monster * next;
  char name;
  struct yx_uint16 pos; };

uint16_t rrand(char, uint32_t);
struct Map init_map ();
void save_game(struct World *);
void record_action (char);
struct yx_uint16 mv_yx_in_dir (char, struct yx_uint16);
char yx_uint16_cmp (struct yx_uint16, struct yx_uint16);
void next_turn (struct World *);
void update_log (struct World *, char *);
void move_player (struct World *, char);
char is_passable (struct Map *, uint16_t, uint16_t);
void player_wait(struct World *);
void toggle_window (struct WinMeta *, struct Win *);
void scroll_pad (struct WinMeta *, char);
void growshrink_active_window (struct WinMeta *, char);
void map_scroll (struct Map *, char);
unsigned char meta_keys(int, struct World *, struct WinMeta *, struct Win *, struct Win *, struct Win *, struct Win *);
