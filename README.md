# neo-myth-menu

This is the menu the console will show when you boot the Neo Myth flash cart. Code for the MD, N64, PC-E and SNES menus has been committed.


## News

* 2013-06-22: v0.60 of the SNES menu has been released.
* 2012-05-25: v2.8 of the MD Myth DX menu has been released.
* 2012-02-13: v2.7 of the MD Myth DX menu has been released.
* 2011-11-26: v0.56 of the SNES menu has been released.
* 2011-10-16: v1.0 of the MKIII/SMS menu has been released.
* 2011-08-21: v2.4b1 of the N64 menu has been released.
* 2011-03-08: v2.2 of the N64 menu has been released.
* 2011-01-23: v0.55 of the SNES menu has been released.
* 2011-01-13: v0.54 of the SNES menu has been released.
* 2011-01-07: v0.53 of the SNES menu has been released.
* 2010-12-21: v0.51 of the SNES menu has been released.
* 2010-12-21: v2.5 of the MD Myth DX menu has been released.
* 2010-11-10: v2.4 of the MD Myth DX menu has been released.
* 2010-08-31: v1.9.1 - a quick bug-fix to v1.9.
* 2010-08-30: v1.9 of the N64 menu has been released.
* 2010-08-21: v1.8 of the N64 menu has been released.
* 2010-08-18: v1.7 of the N64 menu has been released.
* 2010-08-11: v1.6 of the N64 menu has been released.
* 2010-08-09: v1.10 of the PC-E menu has been released.
* 2010-06-07: v0.26 of the SNES menu has been released.
* 2010-05-12: v0.25 of the SNES menu has been released.


## Usage

### SNES menu

#### Using NEO2 Ultra Menu / NEO2 Manager

* Unzip the downloaded archive and copy the file NEOSNES.BIN to your NEO2 Manager or NEO2 Ultra Menu directory (e.g. c:\NEO2 Ultra Menu\).
* Start the NEO2 Manager / Ultra Menu with the SNES Myth attached.
* The menu is updated by either burning one or more games to the cart, or by doing a Menu Write on the Memory tab.


## Build instructions

### SNES menu

* First get the snes-sdk (see [this topic](http://www.neoflash.com/forum/index.php/topic,6025.0.html) for Linux, and [this topic](http://www.neoflash.com/forum/index.php/topic,6108.0.html) for Windows.
* On Linux you're also going to need Wine.
* Check out the neo-myth-menu code.
* Set the SNESSDK environment variable to point out the snes-sdk if you're on Linux.
* Use make all to build on Linux. Use make_menu.bat on Windows.
