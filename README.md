
# THE LAST EICHHOF           

Original version Alpha-Helix 1993.
Ported to Allegro library Gavin Smith 2014.

This repo is forked from loonycyborg's repo and minor fixes by WildPenguin 2026.

## INSTALLATION

For Arch-based distributions, there is a PKGBUILD in AUR. Should work on any such
distro, if they pack allegro4, as the game has no other dependencies.

On a Unix-like system, run ./configure followed by make install. High score and
config will be put in user home directory (according to `XDG_CONFIG_HOME` or 
`HOME` variables, most probably in $HOME/.config/lastbeer).

If you want a global high score file, comment out the relevant
line in Makefile.am.

You need Allegro 4, on Debian derivatives it comes as liballegro4-dev.

There isn't a lot of configuration but you may try `-x NUM` `-y NUM` and 
`-X NUM` `-Y NUM` parameters to set one and the other resolution, respectively =).
The game wil try to autodetect two sensible resolutions based on your desktop 
resolution on the first run.

The resolution(s) will be saved so you don't need to specify resolutions every time.
(you need to toggle with ALT+ENTER or set something in options to trigger save).

Some command line options will toggle settings (and they are saved in the configuratio
file). This is a bit weird and all but POSIX. I am being a bit lazy. This is subject to
change =)

## COMMAND LINE OPTIONS

Run with `./beer -?` to check these at any time.

```
Syntax:           BALLER [options]
  /ns             Play without sound.
  -x 960 -y 660   set window resolution,
  -X 3440 -Y 1440 set another resolution,
  -r              Reset / auto-detect window resolution.
  -R              Reset / auto-detect another resolution.
  -s a|i|s        Use aspect (default), integer or stretch scaling.
  -v              Toggle verbosity to STDOUT.
  -l              Toggle speedrunning lap times to STDOUT.
```

## A BIT ABOUT THE LAST EICHHOF 

The game uses 320 x 240 (kind of), but the intro uses 640 x 350.
These are both old VGA/EGA resolutions. 640 x 350 didn't have a square PAR 
originally. Actually, the game uses some weird  slightly-less-tall than 240 
resolution (320 x 220), which was commonplace with other DOS games, too (so, 
not that weird actually). I haven't checked on a real hardware but this is probably 
what it actually does (at least this is the window DOSBox creates, too).

I'm assuming it originally intended to have square PAR, but on a real CRT, you 
could adjust to fill the whole 4:3, if you so desired - achieving non-1:1 PAR. 
Many DOS game have similar peculiarities (compared to modern games).

The current implementation will always use correct aspect ratio, ASSUMING Blick
intro is 4:3 (PAR ~1,37), but in-game PAR is 1:1. But some other assumptions 
could be just as valid, and user might just have preferences.

This means you will get pillarboxing (or letterboxing) if your resolution is 
not a multiple of the game resolution 16x11 (320x220), or 4:3 for the Blick intro 
(but the Blick intro graphics has a black backdrop anyways...).

You can achieve a non-16:11 resolution (non-1:1 PAR) in-game by setting the scaling
to stretch (parameter `-s s`) and any arbitrary window resolution. With aspect
scaling (parameter `-s a`), it is not possible to set other than 1:1 PAR currently
(a custom PAR would be veru easy to implement, though) - so you can not set custom
pillar/letterboxing. Play with the window somehow =).

If you are not satisfied with the scaling options the game currently offers,
set a resolution of 320 x 220 (or whatever you like) and use gamescope. However
gamescope causes some very minor glitches with some timing (mainly effects the
shop money decrement animation, which relies on drawing being "slow"?, I haven't
noticed any in-game effects).

## KNOWN ISSUES

### About Allegro4 and it's limitations

This game uses Allegro 4. That library is ancient and unmaintained (Allegro5 is 
active).

Especially when playing on Wayland, fullscreen doesn't work properly; the game
has trouble registering keystrokes (no problems under X.Org).

As a workaround, the game currently uses just two resolutions, Set the other to
a small one and the other to a large one (like your desktop) and use your VM to 
remove decorations to get a "fake fullscreen", aka "borderless fullscreen 
window" in other games. I don't know how to remove window decorations (possibly
doable, at least in some hacky way, but not platform independent). If you are
running X.Org, you may want to try true fullscreen.

### Differences from the original DOS version

The port is almost faithful, gameplay-wise. There are small differences and the
port is slightly easier. The animation is smoother - however, weapons are also a
bit different. At least the "Pony" weapon reaches further and the angle is
wider. I also have a hunch the fire rate is slightly faster.

I'm not sure these differences are worth fixing, as it's very much playable and
fun as it stand. But pull requests aiming at making this port more faithful
to the original are welcome =)

## COPYING

Original source code for MS-DOS released by Dany Schoch under the following
licence:

-----------------------------------------------------------------------------
Licence Agreement
------------------
Do what ever you want with this code.
Use it in your code, change it, delete it.
HOWEVER: Don't blame me if it doesn't work the way it should.
-----------------------------------------------------------------------------

Game data copyrighted to the original authors and freely distributable,
under the same license.

C source code for Allegro port copyright 2014 by Gavin Smith and licensed
under the GNU General Public License version 3.  See the file LICENSE.GPL
included with the program.

-----------------------------------------------------------------------------

## WEAPONS

```
NAME
PRICE
ENERGIZATION PERIOD / POWER
DESCRIPTION

EICHHOF LAGER 58CL
3 / 2
The main weapon you start with and keep throughout the game

STANGE
3.00
6 / 3
Single large white shot

PONY
3.35
4 / 2
Two angled streams of bouncing shots

BARBARA BRAEU -
3.20
4 / 2
Two angled streams

DUNKEL - 
3.00 
5 / 2
Heat-seeking shots

From the menu card -

CAN 33CL        2.40    6 / 2
	Shoot out waves of energy from its sides
CHEUBELI        2.60    5 / 3
	Shoot downwards
POKAL           4.00    20 / 6
	Explode into 4 smaller shots
XENON 2 CANNON  5.40    8 / 6
	Single powerful missile
```

-----------------------------------------------------------------------------

## LEVELS

### 0 Easy Start

### 1 Feldschloesschen

Soon after defeating the blimp miniboss, there will be a big blue crate
that throws out exploding bottles.  Watch out for the brown shards, and turn up
the brightness of your monitor if necessary.  When the crate rises from the
bottom of the screen, stay away from the bottom of the screen where the green
bottles will rush in, watching out for the looping bottle on the left and its
homing shot.  The centre left is a good place to be.

The two end bosses are quite hard.  When there are two crates on either
side throwing out exploding bottles, try to stay close to the blue crates,
dodge down for each shard and get back up as soon as possible.	Once you have
destroyed one of the crates, take a path to the other side in the upper half of
the screen so that the shards will be coming from below.  It is easier to dodge
shards coming at you if they are coming from above or below because the Eichhof
bottle is narrower than it is tall.  A fairly fool-proof way is to have
selected the Pony weapon and to shoot the exploding bottles with Eichhof while
shooting the crates with the Pony.  If all else fails, try to stay alive long
enough for the final boss to appear.  When you die again, you will skip
straight to the final boss.

Destroy one of the small blue crates first, then move to the other
side.  Watch out for the large, slow round white missiles that are aimed at
you.  You should try either to be high up at the sides or near the bottom
at centre when they are released, but avoid the corners.

### 2 Weissbier

When going past the exploding barrels, try to remember which ones you
have shot the most to anticipate which will explode first.

For final boss, start at the bottom right of the screen. Then move to
the top right and let the row of beers shoot underneath you.  Then back to the
bottom right.  Circle round the boss to the bottom left, avoiding the arc of
small bottles it shoots out.

### 3 No More Cocktails

For the final three bosses, circle around whichever one is on the
bottom right.

### 4 The Morning After

Spend any left-over money on extra lives.

The part where two toilets appear at the bottom left and spit out
bottles is very hard.  There will be two runs of green bottles.  The first
will go up slightly before reaching the right of the screen and the second
will go up along the right edge before circling round the top.  Position
yourself in between them.

When navigating the field of flying toasters, don't fire your weapons.
This will avoid the counter-shots when each toaster is destroyed.  After that,
watch out for the two shots spat high out the top of the small pink cup.
This cup takes a lot of damage but is easy to ignore.  You can sometimes duck
between the cup and its shots.  It will go off screen
by itself eventually.

Immediately after there are three exploding apple cores coming at you,
wait for the alarm clocks to appear at the right side of the screen.  Then
position yourself under the bottom one.  This should allow you to destroy them
quickly and avoid their shots.

The bottom right of the screen is a good place to be for the second phase of 
the final boss to dodge targeted shots and destroy the spawned alarm clocks.


## General advice:

If you cannot catch an enemy up because they are moving the same speed as you,
try moving towards them to give your bullets time to catch up.

Position your ship in anticipation of homing shots so they will go where you
want to make them easy to dodge.  Honing shots aim towards the top (the bottle
cap) of your main weapon.  Therefore the top left and top right of the screen
are good places for them to go.

-----------------------------------------------------------------------------

## Changes:

### 2.0 version released on Sourceforge

- further worked on by Adam Borowski and loonycyborg on github

### 2.11w - a fork from loonycyborg's fork with minor fixes by WildPenguin

 **There are no gameplay-changing changes in this fork (so far).**

#### Sound improvements:

- No more clipping (on most systems)
- Sound is a bit more quiet. Allegro4 does not have mixing capabilities, this is how it handles stuff.
- Panning sound effects (option, enabled per default)!

#### Video improvements:

- Two toggleable, user-settable resolutions
- Real fullscreen is buggy but can be enabled (-f) - usable only under X.Org
- Scaling modes (aspect/integer/stretch)

#### File handling changes:

- Configuration and highscore files are now separate; highscore file could be installed separately for multi-user systems
- Game looks for files in most common locations; easier to port to other systems?

#### Configuration changes

- Stereo (pannig sfx) can be set in options
- some options only on command line. See 'beer /?'

#### Miscellaneous

- Another hidden cheat! (see source)
- Useful output for speedrunners in STDOUT
- When buying a new weapon and cancelling, the money is not lost for nothing anymore
- Some other minor fixes too insignificant and numerous to list here

### 2.12w

- ADPCM decoding done properly now (I hope!)
- A similar check to highscore file as to configuration file
  (in case it's format changes, the game should detect it)
- Command line parsing / helpstring fixes
- Cheaters should not be able to enter highscores.
- Sound damping default is now 2. I believe incorrect ADPCM decoding increased the likelihood of clipping -> less likely now. Increase the value if you get clipping, or maybe even decrease to 1!

-----------------------------------------------------------------------------
## WEB LINKS
-----------------------------------------------------------------------------

- SourceForge home https://sourceforge.net/projects/lasteichhof/
- Forked from: https://github.com/loonycyborg/TheLastEichhof
- Alpha-Helix used to have a website at: http://www.ife.ee.ethz.ch/~ammann/alphahelix/eichhof/.  
- The Internet Archive has an archived version at: https://web.archive.org/web/20050829001932/http://www.ife.ee.ethz.ch/~ammann/alphahelix/eichhof/
