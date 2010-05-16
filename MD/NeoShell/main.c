short int gResetMode = 0x0000;          /* 0x0000 = reset to menu, 0x00FF = reset to game */
short int gGameMode = 0x00FF;           /* 0x0000 = menu mode, 0x00FF = game mode */
short int gWriteMode = 0x0000;          /* 0x0000 = write protected, 0x0002 = write-enabled */
short int gYM2413 = 0x0001;             /* 0x0000 = YM2413 disabled, 0x0001 = YM2413 enabled */

short int gCardType;                    /* 0 = 512 Mbit Neo2 Flash, 1 = other */
short int gCursorX;                     /* range is 0 to 63 (only 0 to 39 onscreen) */
short int gCursorY;                     /* range is 0 to 31 (only 0 to 27 onscreen) */

int main(void)
{
    return 0;
}
