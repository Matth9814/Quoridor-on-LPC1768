#include "player.h"

player_t Player(pid_t id, uint8_t num_walls, uint16_t color, pos_t pos){
	player_t p;
	p.id = id;
	p.num_walls = num_walls;
	p.color = color;
	p.pos.row = start_pos[id].row;
	p.pos.col = start_pos[id].col;
	return p;
}
