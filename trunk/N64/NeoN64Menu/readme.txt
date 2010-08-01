========================================================================
              Neo N64 Myth Menu v1.1 by Chilly Willy
========================================================================
The Neo N64 Myth Menu uses libdragon, by Shaun Taylor. Many thanks for
his work on this fine SDK for the N64.

My thanks, also, to Dr.neo, SivenYu, Conle, madmonkey, and mic.
========================================================================

Installation: Copy NEON64.v64 and NEON64MF.v64 to where your N64 rom
images are. In the Neo2 Pro Manager on your PC, set the boot type to
"TypeA-2: N64 U2", set the BIOS Path A-2 to the NEON64.v64 image, then
click "Burn" to set the menu in the N64 Myth menu flash.

That menu needs to be written to the N64 menu flash in order for some of
the extra features of v1.1 to work. You can burn the NEON64MF.v64 rom
image to the GBA flash cart menu flash, setting the path to the image in
the BIOS Path A-1. If you leave the boot type on TypeA-1, the menu in the
N64 menu flash will detect the menu in the GBA menu flash and run it.
This allows you to do updates to the GBA menu flash, leaving the N64 menu
flash alone unless there is a major update. You could also boot only the
GBA menu flash directly, but you will lose some features of the menu if
you do so.

Similarly, you can put the NEON64SD.v64 on your SD card. Copy it to
"/.menu/n64/NEON64SD.v64" - create the directories if they don't already
exist. Spelling and capitalization matter, so get it right! When the menu
in either the N64 menu flash or the GBA menu flash find that file, they
will boot it, assuming it to be the most up to date version. This is the
easiest way to handle minor updates - put v1.1 in the N64 menu flash and
then any new updates on the SD card.

========================================================================

Usage: The menu boots into the Game Flash browser. At the top of the
display is the title - this shows the version of the menu.

Below that is a list of (up to) ten titles. This is the game list. It
can handle 1024 games - scroll using the dpad to see all the games. Only
ten are shown at any one time. The currently selected game is shown in
light gray, while all the rest are shown in green.

Below the game list is the game info display. This shows the name in the
header, the country code, the manufacturer's ID, and the cart ID. If you
press Z, this changes to show the game options instead of the game info.
When the game options are shown, the red option may be changed using the
cpad.

Underneath the game info is the complete filename for the entry. Since
the menu can only show up to 36 characters for the entry in the game
list, this area shows the complete name to better determine one game
from another. Names can be up to 56 characters in the flash.

Under that is the hardware info line. This shows the hardware revision
of the N64 Myth (CPLD), the GBA ASIC ID (CART), and the GBA flash type
(FLASH). This may be handy when reporting problems.

Finally, the last few lines are a help display for the controller usage.

Pressing Start switches you to the SD card browser. It is virtually the
same as the Flash browser, except that the game list may include directory
entries. Pressing A or B while a directory is high-lighted will enter
that directory to show its contents. Any number of subdirectories can
be browsed, as long as the total path is less than 1023 characters long.
File names can be up to 255 characters long, and are not included in the
length of the path. The total length allowed for both path and file name
combined is 1279 characters. The game info for SD card files is not shown
initially to speed things up. Pause over a file for a second and it will
read and display the game info. Note that while flash games start instantly,
SD card games must be read into memory, and can take a while. Currently,
it take about 50 seconds to load an 8 MByte game, about 1:50 to load a
16 MByte game, and about 3:35 to load a 32 MByte game. Larger games cannot
be loaded at this time as the Neo2-Pro has only 32 MBytes of memory for
loading games into. If you have a Neo2-SD, the maximum is 16 MBytes.

Notes: The byte order of files is important. There are three main types
of N64 rom images: .v64, which has the bytes in a word swapped; .z64,
which does not (this type may also sometimes have a .rom or .bin suffix);
and finally, .n64, which has the bytes in a long swapped. The color of
the game info will tell you if the game data is in the proper byte order
or not. If it is white, the byte order is fine. If it is lavender, it is
not. This doesn't matter when loading from the SD card - the loader will
automatically swap the data as it loads. The only thing it does is slow
the loading down just slightly. It's more of a matter for games in the
game flash. You can only play the game directly from flash when the byte
order is correct (the game info is white). When the game info is lavender,
the game cannot be directly run from the flash. If you are using a Neo2-SD
or Neo2-Pro, the menu will copy the game from the flash to psram. It will
swap the data as needed to allow it to run. If you are on a regular flash
cart, they haven't any psram, so you cannot play games where the game info
is not white. The proper files to burn to flash to be directly playable
are of the .v64 variety.

One new feature of the v1.1 menu if you have a Neo2-SD or Neo2-Pro has to
do with games that won't reset to the game because their CIC type doesn't
match the boot CIC type. If you run them from the SD card, I have code in
the N64 menu flash menu that will rerun the game, allowing you to run with
reset to game. I'm still thinking of how to do this with games in the game
flash. I can do this on the Neo2-SD and Neo2-Pro, but not on plain flash
only carts.

Another new feature is the ability to boot Myth-aware homebrew. This is
tied to the ability to boot menus from anywhere as described above. What
the homebrew author does is set the first eight characters of the rom
header name to "N64 Myth" - the rest of the name doesn't matter. When
those characters are found, the program is run with the hardware unlocked
so that the N64 Myth hardware (particularly the SD card interface) may be
used.

Another new feature has to do with the game options. No longer is a number
displayed - instead, you see what the option stands for. The only two you
can change are the save type, and the CIC type. It is now obvious to what
each option value is set.

========================================================================
                            Controls
========================================================================
DPad Up: move one entry back in the game list
DPad Down: move one entry forward in the game list
DPad Left: move one page (10 entries) back in the game list
DPad Right: move one page forward in the game list

A: run selected entry with the reset mode set to reset back to the menu
B: run selected entry with the reset mode set to reset back to the game

Z: toggle between game info and game options

Start: toggle between the Flash browser and the SD card browser

Left Trigger: switch to the previous background fill pattern
Right Trigger: switch to the next background fill pattern

CPad Up: increase selected game option
CPad Down: decrease selected game option
CPad Left: select previous game option
CPad Right: select next game option
========================================================================

For more info, please visit the NeoFlash forums:
http://www.neoflash.com/forum/index.php

Shaun Taylor's N64 Development web page:
http://www.dragonminded.com/?loc=n64dev/N64DEV

========================================================================
