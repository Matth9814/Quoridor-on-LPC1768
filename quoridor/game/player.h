#ifndef __PLAYER_H
#define __PLAYER_H

#include <stdlib.h>
#include <stdint.h>
#include "GUI.h"

// Coordinates
#define SPACE_DIM	2

typedef enum {P1, P2, NUM_PLAYERS, NO_PLAYER} pid_t;

typedef struct position{
	short int row;
	short int col;
} pos_t;

typedef struct player{
	uint8_t id;
	uint8_t num_walls;
	uint16_t color;
	pos_t pos;
} player_t;

extern const pos_t start_pos[NUM_PLAYERS];
extern const uint16_t color_p[NUM_PLAYERS];

player_t Player(pid_t id, uint8_t num_walls, uint16_t color, pos_t pos);

#endif
