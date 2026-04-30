struct sound_s {
  bool sound;
  bool pan_foes;
  bool pan_sfx;
};

struct resolution_s {
  short X;
  short Y;
};

struct graphic_s {
  struct resolution_s full;
  struct resolution_s window;
  bool fullscreen;
  bool truefullscreen; // per default, just two resolutions
                       // Allegro4 is buggy on Wayland (input breaks down)
                       // But you may still enable this if you are on X.Org
};

static struct beerconfig_s {
  short key_left;
  short key_right;
  short key_up;
  short key_down;
  short key_fire;
  short key_pause;
  struct sound_s ss;
  struct graphic_s res;
} BeerConfigDefault = {
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_SPACE,
    KEY_P,
    { true, true, true },
    { 
      { 1920, 1080},
      { 640, 440 },
      false, false
    }
};

typedef struct beerconfig_s beerconfig;
beerconfig eichcfg;
