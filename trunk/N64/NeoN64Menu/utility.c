#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libdragon.h>
#include <ff.h>

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

extern void debugText(char *msg, int x, int y, int d);
extern void c2wstrcpy(void *dst, void *src);

// CRC code adapted from DaedalusX64 emulator

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

unsigned int CRC_Calculate(unsigned int crc, unsigned char* buf, unsigned int len)
{
    static unsigned int crc_table[256];
    static int make_crc_table = 1;

    if (make_crc_table)
    {
        unsigned int c, n;
        int k;
        unsigned int poly;
        const unsigned char p[] = { 0,1,2,4,5,7,8,10,11,12,16,22,23,26 };

        /* make exclusive-or pattern from polynomial (0xedb88320L) */
        poly = 0L;
        for (n = 0; n < sizeof(p)/sizeof(unsigned char); n++)
            poly |= 1L << (31 - p[n]);

        for (n = 0; n < 256; n++)
        {
            c = n;
            for (k = 0; k < 8; k++)
                c = c & 1 ? poly ^ (c >> 1) : c >> 1;
            crc_table[n] = c;
        }
        make_crc_table = 0;
    }

    if (buf == (void*)0) return 0L;

    crc = crc ^ 0xffffffffL;
    while (len >= 8)
    {
        DO8(buf);
        len -= 8;
    }
    if (len)
        do
        {
            DO1(buf);
        } while (--len);

    return crc ^ 0xffffffffL;
}

int get_cic(unsigned char *buffer)
{
    unsigned int crc;
    // figure out the CIC
    crc = CRC_Calculate(0, buffer, 1000);
    switch(crc)
    {
        case 0x303faac9:
        case 0xf0da3d50:
            return 1;
        case 0xf3106101:
            return 2;
        case 0xe7cd9d51:
            return 3;
        case 0x7ae65c9:
            return 5;
        case 0x86015f8f:
            return 6;
    }
    return 2;
}

int get_cic_save(char *cartid, int *cic, int *save)
{
    // variables
    int NUM_CARTS = 136;
    int i;

    //data arrays
    //char *names[] = {"Worms Armageddon", "Super Smash Bros.", "Banjo-Tooie", "Blast Corps", "Bomberman Hero", "Body Harvest", "Banjo-Kazooie", "Bomberman 64", "Bomberman 64: Second Attack", "Command & Conquer", "Chopper Attack", "NBA Courtside 2 featuring Kobe Bryant", "Penny Racers", "Chameleon Twist", "Cruis'n USA", "Cruis'n World", "Legend of Zelda: Majora's Mask, The", "Donkey Kong 64", "Donkey Kong 64", "Donald Duck: Goin' Quackers", "Loony Toons: Duck Dodgers", "Diddy Kong Racing", "PGA European Tour", "Star Wars Episode 1 Racer", "AeroFighters Assault", "Bass Hunter 64", "Conker's Bad Fur Day", "F-1 World Grand Prix", "Star Fox 64", "F-Zero X", "GT64 Championship Edition", "GoldenEye 007", "Glover", "Bomberman 64", "Indy Racing 2000", "Indiana Jones and the Infernal Machine", "Jet Force Gemini", "Jet Force Gemini", "Earthworm Jim 3D", "Snowboard Kids 2", "Kirby 64: The Crystal Shards", "Fighters Destiny", "Major League Baseball featuring Ken Griffey Jr.", "Killer Instinct Gold", "Ken Griffey Jr's Slugfest", "Mario Kart 64", "Mario Party", "Lode Runner 3D", "Megaman 64", "Mario Tennis", "Mario Golf", "Mission: Impossible", "Mickey's Speedway USA", "Monopoly", "Paper Mario", "Multi-Racing Championship", "Big Mountain 2000", "Mario Party 3", "Mario Party 2", "Excitebike 64", "Dr. Mario 64", "Star Wars Episode 1: Battle for Naboo", "Kobe Bryant in NBA Courtside", "Excitebike 64", "Ogre Battle 64: Person of Lordly Caliber", "Pokémon Stadium 2", "Pokémon Stadium 2", "Perfect Dark", "Pokémon Snap", "Hey you, Pikachu!", "Pokémon Snap", "Pokémon Puzzle League", "Pokémon Stadium", "Pokémon Stadium", "Pilotwings 64", "Top Gear Overdrive", "Resident Evil 2", "New Tetris, The", "Star Wars: Rogue Squadron", "Ridge Racer 64", "Star Soldier: Vanishing Earth", "AeroFighters Assault", "Starshot Space Circus", "Super Mario 64", "Starcraft 64", "Rocket: Robot on Wheels", "Space Station Silicon Valley", "Star Wars: Shadows of the Empire", "Tigger's Honey Hunt", "1080º Snowboarding", "Tom & Jerry in Fists of Furry", "Mischief Makers", "All-Star Tennis '99", "Tetrisphere", "V-Rally Edition '99", "V-Rally Edition '99", "WCW/NWO Revenge", "WWF: No Mercy", "Waialae Country Club: True Golf Classics", "Wave Race 64", "Worms Armageddon", "WWF: Wrestlemania 2000", "Cruis'n Exotica", "Yoshi's Story", "Harvest Moon 64", "Legend of Zelda: Ocarina of Time, The", "Legend of Zelda: Majora's Mask, The", "Airboarder 64", "Bakuretsu Muteki Bangaioh", "Choro-Q 64 II", "Custom Robo", "Custom Robo V2", "Densha de Go! 64", "Doraemon: Mittsu no Seireiseki", "Dezaemon 3D", "Transformers Beast Wars", "Transformers Beast Wars Metals", "64 Trump Collection", "Bass Rush", "ECW Hardcore Revolution", "40 Winks", "Aero Gauge", "Aidyn Chronicles The First Mage", "Derby Stallion 64", "Doraemon 2 - Hikari no Shinden", "Doraemon 3 - Nobi Dai No Machi SOS", "F-1 World Grand Prix II", "Fushigi no Dungeon - Furai no Shiren 2", "Heiwa Pachinko World 64", "Neon Genesis Evangelion", "Racing Simulation", "Tsumi to Batsu", "Sonic Wings Assault", "Virtual Pro Wrestling", "Virtual Pro Wrestling 2", "Wild Choppers"};
    char *cartIDs[] = {"AD", "AL", "B7", "BC", "BD", "BH", "BK", "BM", "BV", "CC", "CH", "CK", "CR", "CT", "CU", "CW", "DL", "DO", "DP", "DQ", "DU", "DY", "EA", "EP", "ER", "FH", "FU", "FW", "FX", "FZ", "GC", "GE", "GV", "HA", "IC", "IJ", "JD", "JF", "JM", "K2", "K4", "KA", "KG", "KI", "KJ", "KT", "LB", "LR", "M6", "M8", "MF", "MI", "ML", "MO", "MQ", "MR", "MU", "MV", "MW", "MX", "N6", "NA", "NB", "NX", "OB", "P2", "P3", "PD", "PF", "PG", "PH", "PN", "PO", "PS", "PW", "RC", "RE", "RI", "RS", "RZ", "S6", "SA", "SC", "SM", "SQ", "SU", "SV", "SW", "T9", "TE", "TJ", "TM", "TN", "TP", "VL", "VY", "W2", "W4", "WL", "WR", "WU", "WX", "XO", "YS", "YW", "ZL", "ZS", "AB", "BN", "CG", "CX", "CZ", "D6", "DR", "DZ", "OH", "TB", "TC", "VB", "WI", "4W", "AG", "AY", "DA", "D2", "3D", "F2", "SI", "HP", "EV", "MG", "GU", "SA", "VP", "A2", "WC"};
    int saveTypes[] = {5, 1, 6, 5, 5, 5, 5, 5, 5, 4, 5, 4, 5, 5, 5, 6, 4, 6, 6, 5, 5, 5, 5, 6, 5, 5, 6, 5, 5, 1, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 1, 5, 4, 5, 5, 5, 4, 6, 1, 5, 5, 5, 4, 5, 5, 6, 5, 6, 5, 5, 6, 6, 1, 4, 4, 6, 4, 5, 4, 4, 4, 4, 5, 5, 1, 1, 5, 6, 5, 5, 5, 5, 4, 5, 5, 5, 4, 1, 5, 5, 5, 5, 5, 5, 1, 4, 5, 5, 5, 1, 5, 6, 1, 1, 4, 5, 5, 5, 5, 6, 1, 5, 1, 5, 5, 5, 1, 1, 5, 5, 1, 1, 6, 6, 6, 4, 5, 6, 5, 5, 5, 1, 1, 5};
    int cicTypes[]  = {2, 3, 5, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 6, 5, 5, 5, 2, 2, 3, 2, 2, 2, 2, 5, 2, 1, 6, 2, 2, 2, 2, 2, 2, 5, 5, 2, 2, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 5, 2, 3, 2, 2, 2, 2, 3, 2, 2, 3, 3, 2, 3, 3, 5, 3, 2, 3, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6, 2, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

    // search for cartid
    for (i=0; i<NUM_CARTS; i++)
        if (strcmp(cartid, cartIDs[i]) == 0)
            break;

    if (i == NUM_CARTS)
    {
        // cart not in list
        *cic = 2;
        *save = 0;
        return 0; // not found
    }

    // cart found
    *cic = cicTypes[i];
    *save = saveTypes[i];
    return 1; // found
}

int get_swap(unsigned char *buf)
{
    char temp[40];
    unsigned int val = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
    switch(val)
    {
        case 0x80371240:
            return 0;                       // no swapping needed
        case 0x37804012:
            return 1;                       // byte swapping needed
        case 0x12408037:
            return 2;                       // word swapping needed
        case 0x40123780:
            return 3;                       // long swapping needed
    }
    sprintf(temp, "unknown byte format: %08X", val);
    debugText(temp, 5, 2, 180);
    return 0;
}

/*
 * Load an image from the rom filesystem, returning a pointer to the array
 * of sprites that hold the image. The width and height of the image in
 * sprites is stored in w and h.
 */
sprite_t *loadImageDFS(char *fname, int *w, int *h)
{
    int size, x, y, n, fd;
    u8 *tbuf;
    u8 *ibuf;
    sprite_t *sbuf;

    fd = dfs_open(fname);
    if (fd < 0)
        return 0;                       // couldn't open image

    size = dfs_size(fd);
    tbuf = malloc(size);
    if (!tbuf)
    {
        dfs_close(fd);
        return 0;                       // out of memory
    }

    dfs_read(tbuf, 1, size, fd);
    dfs_close(fd);

    ibuf = stbi_load_from_memory(tbuf, size, &x, &y, &n, 4);
    free(tbuf);
    if (!ibuf)
        return 0;                       // couldn't decode image

    *w = x / 64;
    *h = y / 16;
    sbuf = (sprite_t*)malloc((*w) * (*h) * (64*16*2 + 4));
    if (!sbuf)
    {
        stbi_image_free(ibuf);
        return 0;                       // out of memory
    }

    for (int iy=0; iy<(*h); iy++)
        for (int ix=0; ix<(*w); ix++)
        {
            color_t *src = (color_t*)ibuf;
            u16 *dst = (u16*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 4);
            for (int j=0; j<16; j++)
                for (int i=0; i<64; i++)
                    dst[i + j*64] = graphics_convert_color(src[ix*64 + i + (iy*16 + j)*x]) & 0x0000FFFF;
            *(u8*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 0) = 64;
            *(u8*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 1) = 16;
            *(u8*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 2) = 2;
            *(u8*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 3) = 0;
        }

    stbi_image_free(ibuf);
    return sbuf;
}

/*
 * Load an image from the SD card, returning a pointer to the array
 * of sprites that hold the image. The width and height of the image in
 * sprites is stored in w and h.
 */
sprite_t *loadImageSD(char *fname, int *w, int *h)
{
    TCHAR wname[1280];
    UINT ts;
    int size, x, y, n;
    u8 *tbuf;
    u8 *ibuf;
    sprite_t *sbuf;
    FIL gSDFile;

    c2wstrcpy(wname, fname);
    f_close(&gSDFile);
    if (f_open(&gSDFile, wname, FA_OPEN_EXISTING | FA_READ) != FR_OK)
        return 0;                       // couldn't open image

    size = gSDFile.fsize;
    tbuf = malloc(size);
    if (!tbuf)
    {
        f_close(&gSDFile);
        return 0;                       // out of memory
    }

    f_read(&gSDFile, tbuf, size, &ts);
    f_close(&gSDFile);

    ibuf = stbi_load_from_memory(tbuf, size, &x, &y, &n, 4);
    free(tbuf);
    if (!ibuf)
        return 0;                       // couldn't decode image

    *w = x / 64;
    *h = y / 16;
    sbuf = (sprite_t*)malloc((*w) * (*h) * (64*16*2 + 4));
    if (!sbuf)
    {
        stbi_image_free(ibuf);
        return 0;                       // out of memory
    }

    for (int iy=0; iy<(*h); iy++)
        for (int ix=0; ix<(*w); ix++)
        {
            color_t *src = (color_t*)ibuf;
            u16 *dst = (u16*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 4);
            for (int j=0; j<16; j++)
                for (int i=0; i<64; i++)
                    dst[i + j*64] = graphics_convert_color(src[ix*64 + i + (iy*16 + j)*x]) & 0x0000FFFF;
            *(u8*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 0) = 64;
            *(u8*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 1) = 16;
            *(u8*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 2) = 2;
            *(u8*)((u32)sbuf + ((iy*(*w) + ix) * (64*16*2 + 4)) + 3) = 0;
        }

    stbi_image_free(ibuf);
    return sbuf;
}

void drawImage(display_context_t dcon, sprite_t *sprites, int w, int h)
{
    int x, y = 0;

    rdp_sync(SYNC_PIPE);
    rdp_set_default_clipping();
    rdp_enable_texture_copy();
    rdp_attach_display(dcon);
    // Draw image
    for (int j=0; j<h; j++)
    {
        x = 0;
        for (int i=0; i<w; i++)
        {
            rdp_sync(SYNC_PIPE);
            rdp_load_texture(0, 0, MIRROR_DISABLED, (sprite_t*)((u32)sprites + ((j*w + i) * (64*16*2 + 4))));
            rdp_draw_sprite(0, x, y);
            x += 64;
        }
        y += 16;
    }
    rdp_detach_display();
}
