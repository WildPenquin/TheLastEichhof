struct sound_s {
  bool sound;
  bool pan_foes;
  bool pan_sfx;
};

struct resolution_s {
  short X;
  short Y;
};

enum scalingmode {
  ASPECT,
  INTEGER,
  STRETCH
};

struct misc_s {
  bool speedrun;       // print speedrunning times to STDOUT
  bool verbose;        // print a bit more verbose stuff to STDOUT
  char windowtitle[30];// Window title, set to something fun
};

struct graphic_s {
  struct resolution_s full;
  struct resolution_s window;
  bool fullscreen;
  bool truefullscreen; // per default, just two resolutions
                       // Allegro4 is buggy on Wayland (input breaks down)
                       // But you may still enable this if you are on X.Org
  enum scalingmode scale; 
};

#define CFG_REVISION PACKAGE_NAME"21_______" // change if struct has been changed!

static struct beerconfig_s {
  char versionstring[20];
  short key_left;
  short key_right;
  short key_up;
  short key_down;
  short key_fire;
  short key_pause;
  struct sound_s ss;
  struct graphic_s res;
  struct misc_s misc;
} BeerConfigDefault = {
    "DEFAULT",
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_SPACE,
    KEY_P,
    { true, true, true }, // sound
    { 
      { 0 , 0 }, // xmodeasm will auto-detect
      { 0 , 0 },
      false, false,
      ASPECT
    },
    { true, false,
      "Keppana viimeinen" }
};

typedef struct beerconfig_s beerconfig;
beerconfig eichcfg;
