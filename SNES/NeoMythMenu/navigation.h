#ifndef _NAVIGATION_H_
#define _NAVIGATION_H_

#define NUMBER_OF_GAMES_TO_SHOW 9

typedef struct
{
	u16 count;
	u16 firstShown;
	u16 highlighted;
} gamesList_t;

extern u16 read_joypad();

extern void move_to_next_game();
extern void move_to_next_page();
extern void move_to_previous_game();
extern void move_to_previous_page();

extern void update_game_params();
extern void print_games_list();

#endif
