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
  uint16_t y;
  uint16_t x; };

struct Monster {
  uint16_t y;
  uint16_t x; };

uint16_t rrand(char, uint32_t);
uint16_t read_uint16_bigendian(FILE * file);
void write_uint16_bigendian(uint16_t x, FILE * file);
uint32_t read_uint32_bigendian(FILE * file);
void write_uint32_bigendian(uint32_t x, FILE * file);
void save_game(struct World *);
void toggle_window (struct WinMeta *, struct Win *);
void scroll_pad (struct WinMeta *, char);
void growshrink_active_window (struct WinMeta *, char);
struct Map init_map ();
void map_scroll (struct Map *, char);
void next_turn (struct World *);
void update_log (struct World *, char *);
char is_passable (struct Map *, uint16_t, uint16_t);
void record_action (char);
void move_player (struct World *, char);
void player_wait(struct World *);
unsigned char meta_keys(int, struct World *, struct WinMeta *, struct Win *, struct Win *, struct Win *, struct Win *);
