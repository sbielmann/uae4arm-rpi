UAE4ARM-RPI Gameshell edition
=============================


Version 3.5.1


Description

Amiga emulator for ClockWorkPi Gameshell. Using UAE4ARM-RPI made for
Raspberry PI, which by itself is based on UAE emulator for Pandora.

This emulator is not part of RetroArch / Libretro, however the default
installation will place it under Retro Games and the default controls will make
use of the Gameshell buttons the same way as RetroArch does. More over, you may
customize the button keys on Gameshell side, and the button function or key
codes the Amiga will receive when Gameshell buttons are pressed.

This readme is about the Gameshell specific topics, additional details may also
be found in Readme_Pandora.txt


Index

1. Installation
2. Files
3. Configuration
   3.1 Alco Copter on A500
   3.2 Monsters of Terror on A1200
   3.3 Barbarian Plus on A1200
   3.4 TimeGal on CD32
   3.5 Extra controls
   3.6 Default configuration settings
4. Controls
   4.1 Gameshell buttons
   4.2 Remap Gameshell buttons
   4.3 Customize Amiga controls
5. Technical notes


------------------------------------

1. Installation

On Gameshell you have install an additional package, then create a couple of
directories, copy the Launcher configuration file and icon for the emulator.
Both files may be found in this directory here.

Connect to your Gameshell with SSH and enter the following command, a single
line, you will have to confirm the installation with Y key:

sudo apt-get install libguichan-sdl-0.8.1

Copy paste the following 2 lines to your SSH console:
 
mkdir /home/cpi/apps/launcher/Menu/GameShell/20_Retro\ Games/Amiga
mkdir /home/cpi/games/Amiga

Copy the file action.config to:
/home/cpi/apps/launcher/Menu/GameShell/20_Retro Games/Amiga/action.config

Copy the file Amiga.png to:
/home/cpi/apps/launcher/skin/default/Menu/GameShell/20_Retro Games/Amiga.png

Now you have to either restart your Gameshell, or start RetroArch and quit
it again, so that the new launcher menu is reloaded.

In Retro Games you will now find the new entry Amiga, with own boing ball icon.
As soon as you have uploaded ROM, floppy and UAE configuration files to the
Amiga games directory, you will see UAE configuration files listed here. More
details about this in next section.

First time you select a game here, Gameshell will ask whether it should setup
the game engine automatically. Confirm with Yes, the Amiga emulator will then
be downloaded and installed.


------------------------------------

2. Files

Three things are necessary to play a game, the kickstart ROM file, the game
floppy as ADF file and a UAE configuration file.

ROM files may be purchased here:
    https://www.amigaforever.com/
    Or you use transADF to take a copy of the ROM on your real Amiga

ADF files of floppies can be made with:
    http://aminet.net/package/disk/misc/TransADF
    Some games may also be downloaded legally comercially or for free from
    internet.

UAE files are simple text files describing the configuration of the emulation
    for the game, see section Configuration for examples.

Upload these files with Tiny Cloud to your Gameshell, they all belong into
the directory:

games/Amiga


------------------------------------

3. Configuration

You will find a couple of example UAE configuration files in this directory.
For the free games Alco Copter on A500, Monsters of Terror and Barbarian Plus
on A1200 and Time Gal on CD32. Also the commercial games Hybris and Turrican2.

The examples use ROM files and ROM key from Amiga Forever. You may as well
take your own ROM files, e.g. made with TransROM, in that case no ROM key
is needed.

UAE configuration files have an option to point to the ROM key file, however
this is optional. UAE will search for the file rom.key at the same location
where you have the put the ROM files, hence find it by itself. If rom.key
is located somewhere else use:

kickstart_key_file=/home/cpi/games/Amiga/rom.key

The minimum UAE configuration file has usually these 3 options in it:

model, the Amiga model, e.g. A500, A1200, CD32
kickstart_rom_file, path to ROM file
floppy0, path to floppy disk file

Once ROM, ADF and UAE files have been uploaded to your Gameshell, head to
Retro Games, Amiga and scan for new files. You will see a list of all UAE
configuration files, select and start the one you are interested in.

3.1 Alco Copter on A500

What you need:
- amiga-os-130.rom and rom.key
- Alco Copter game, alcocopter.adf file, download from:
  http://flashtro.com/alco-copter-v-1-0/
- AlcoCopter.uae

The UAE configuration tells that we want an A500, with OS 130 and use
alcocopter.adf as floppy. Important is that it requires also number of floppies
be set to 1, else Alco Copter will not run. The reason for this however is
unknown:

chipset_compatible=A500
kickstart_rom_file=/home/cpi/games/Amiga/amiga-os-130.rom
floppy0=/home/cpi/games/Amiga/alcocopter.adf
nr_floppies=1

3.2 Monsters of Terror on A1200

What you need:
- amiga-os-310-a1200.rom and rom.key
- Monsters of Terror game, monsters10.adf file, download from:
  http://www.lazycow.de/monsters/
- Monsters.uae

The UAE configuration file tells that we want an A1200, with OS 310 for A1200
and use monsters10.adf as floppy:

chipset_compatible=A1200
kickstart_rom_file=/home/cpi/games/Amiga/amiga-os-310-a1200.rom
floppy0=/home/cpi/games/Amiga/monsters10.adf

3.3 Barbarian Plus on A1200

What you need:
- amiga-os-310-a1200.rom and rom.key
- Barbarian Plus game, download from:
  http://aminet.net/package/game/2play/Barbarian_Plus_ADFs_1_0
- Unpack the LHA archive and rename the ADF files, e.g. from
  Barbarian Plus 1.adf to barbarian_plud_1.adf
- BarbarianPlus.uae

The UAE configuration file tells that we want an A1200, with OS 310 for A1200
and use the 4 first floppies of Barbarian Plus. It also tells that the Amiga
has 4 floppy drives. For instance there is no way however to swap to floppies
5 and 6 of this game:

model=A1200
kickstart_rom_file=/home/cpi/games/Amiga/amiga-os-310-a1200.rom
nr_floppies=4
floppy0=/home/cpi/games/Amiga/barbarian_plus_1.adf
floppy1=/home/cpi/games/Amiga/barbarian_plus_2.adf
floppy2=/home/cpi/games/Amiga/barbarian_plus_3.adf
floppy3=/home/cpi/games/Amiga/barbarian_plus_4.adf

3.4 TimeGal on CD32

What you need:
- amiga-os-310-cd32.rom, amiga-os-310-cd32-ext.rom, and rom.key
- Time Gal game, TimeGal.iso file, download from:
  http://unofficial-cd32-ports.blogspot.com/2017/03/001-time-gal-reimagine.html
- TimeGal.uae

The UAE configuration file tells that we want a CD32, with OS 310 for CD32
and use TimeGal.iso as CD-ROM ISO image. We also tell where to locate the flash
RAM file, that will be created automatically.

model=CD32
kickstart_rom_file=/home/cpi/games/Amiga/amiga-os-310-cd32.rom
kickstart_ext_rom_file=/home/cpi/games/Amiga/amiga-os-310-cd32-ext.rom
flash_file=/home/cpi/games/Amiga/cd32.nvr
cdimage0=/home/cpi/games/Amiga/TimeGal.iso,image

3.5 Extra controls

Some games need more than just the normal Joystick input used for directions
and the fire button. Hybris for example uses the RETURN key to expand the ship
into a new power configuration, and the SPACEBAR is used for smartbombs.
Or Turrican 2 uses the SPACEBAR to activate two energy lines.

To turn custom controls on use:

pandora.custon_controls=1

For Hybris we could map smartbombs to Y key, and power configuration to A key:

pandora.custom_a=68
pandora.custom_y=64

For Turrican have energy line on Y key with:

pandora.custom_y=64

More details on controls and customization in Controls section.

3.6 Default configuration settings

UAE provides a large set of configuration options, among them here the default
values used for Gameshell unless you set other values inside the configuration
files:

gfx_width=320
gfx_height=240
gfx_linemode=none
use_gui=no
key_for_quit=27
input.joymouse_speed_analog=4
joyport0=none
joyport1=joy0

Graphic output is set to low resolution, this matches 320 x 240. If a game is
made exclusively for the mouse you may use mouse1 like this in it's
configuration file:

joyport0=mouse1
joyport1=none


------------------------------------

4. Controls

The controls are mapped to match the default keypad configuration of Gameshell,
see following link for these defaults:

https://github.com/clockworkpi/Keypad

The Amiga emulator gives you full control over the mapping of the Gameshell
buttons, in case you modified it. And also let's you customize the actions
and keys that are send to the Amiga when you press a Gameshell button.

4.1 Gameshell buttons

This lists the Gameshell buttons and how the are configured to work with the
emulator by default.

MENU   : exit emulator go back to Gameshell menu
Shift  : unused for now, however sends shift key to Amiga
Select : switch between joystick and mouse input
Start  : left mouse button when in joystick mode
A      : left mouse button when in mouse mode
         GREEN button on CD32 controller
B      : right mouse button when in mouse mode
         2nd fire button when in joystick mode
         BLUE button on CD32 controller
X      : 1st fire button when in joystick mode
         RED button on CD32 controller
Y      : YELLOW button on CD32 controller
L1     : left shoulder button
L5     : right shoulder button
L2-L4  : unused for now

4.2 Remap Gameshell buttons

If you have a non default mapping for your Gameshell buttons, then you may tell
the emulator which key's that you have mapped to the buttons with the following
configuration options:

pandora.key_for_a
pandora.key_for_b
pandora.key_for_x
pandora.key_for_y
pandora.key_for_l
pandora.key_for_r
pandora.key_for_up
pandora.key_for_down
pandora.key_for_right
pandora.key_for_left
pandora.key_for_select
pandora.key_for_start

For example you have swapped X and Y buttons you would need to add:

pandora.key_for_x=107
pandora.key_for_y=117

107 is u key, and 107 is i key. A full list of all keys and their corresponding
number may be found here:

https://www.libsdl.org/release/SDL-1.2.15/include/SDL_keysym.h

IMPORTANT NOTE: Key mappings must be done BEFORE custom button mappings.

4.3 Customize Amiga controls

If you would like to customize the Amiga controls, meaning what happens on the
Amiga side when pressing a specific Gameshell button, then use the following
configuration options:

pandora.custom_controls
pandora.custom_up
pandora.custom_down
pandora.custom_left
pandora.custom_right
pandora.custom_a
pandora.custom_b
pandora.custom_x
pandora.custom_y
pandora.custom_l
pandora.custom_r
pandora.custom_select
pandora.custom_start

First thing is to tell that you would like to customize the controls with:

pandora.custom_controls=1

For example send RETURN key to Amiga when pressing A button then use:

pandora.custom_a=68

Number 68 in decimal equals to the hexa decimal number 44. A full list
of all Amiga key codes may be found in file:

https://github.com/Chips-fr/uae4arm-rpi/blob/master/src/include/keyboard.h

Like for example space bar or return key:

AK_SPC 0x40
AK_RET 0x44

Use your computers calculator or any online service that provides ways to
convert between hexadecimal (base 16) and decimal (base 10) numbers.

Special codes apply when you would like to remap joystick or mouse controls,
e.g. have 1st fire button of joystick on Y button:

pandora.custom_y=-3

Full list of joystick and mouse button codes:

mousebutton left    -1
mousebutton right   -2
joybutton one       -3
joybutton two       -4
joy up              -5
joy down            -6
joy left            -7
joy right           -8
CD32 green          -9
CD32 yellow         -10
CD32 play           -11
CD32 ffw            -12
CD32 rwd            -13


------------------------------------

5. Technical notes

The menu UI is made for a resolution of 800 x 600, so it does not fit on
Gameshell yet. Even more, without a virtual keyboard, it will be hard to
enter a name for new configurations. Easiest way at the moment is to connect
with SSH in X forwarding mode and let the menu be shown on your computer
instead.

When working with menu, you will also need to install the data directory,
by default that would be:

/home/cpi/apps/emulators/data

And copy with-menu-action.config over your Amiga/action.config, and copy
uae4armsh to:

/home/cpi/apps/emulators

The data directory contains fonts and icons needed by the menu, and uae4arm
requires to be started right where it's data sub directory is located.
That is why uae4armsh first does a cd, then start uae4arm.


Although there is no problem to compile uae4arm with the menu resolution
set to 320 x 240 for Gameshell, the UI layout itself would need to be
overworked first. The components are of course still placed for 800 x 600
and for instance not really useful that way on Gameshell.


PAL screen mode is 320 x 256, however Gameshell has only 320 x 240. This means
that the lower 16 pixels are cut off. NTSC however with 320 x 240 does not have
this issue. E-UAE could have a solution for this that might also be implemented
here in the future. Also uae4arm-rpi has some kind of screen resizing for
Raspberry Pi implemented, but not tested yet.

Volume control is not implemented. Could have a look at uae4arm-libretro which
has it implemented, however of course with Libretro as backend. Maybe this is
just an SDL audio volume feature.


Customizing the Gameshell button key mapping is not implemented in UI yet.


Maybe using a second joystick over bluetooth could be an idea for 2 player
games.
