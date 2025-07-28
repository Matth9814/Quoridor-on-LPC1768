#ifndef __GUI_H
#define __GUI_H

#include <stdint.h>

// Colors
#define	GrayBlue			 	0x528D
#define	GrayGreen				0xB614
#define	DarkPurple		 	0x2107
#define	Beige					 	0xEF7A
#define LightOrange		 	0xDE31

// Game colors
#define COLOR_BOARD 		GrayBlue
#define COLOR_GRID			GrayGreen
#define COLOR_BG 				DarkPurple
#define COLOR_TEXT			GrayGreen

// Cells
#define NUM_CELLS_X		7			// Cells per ROW
#define NUM_CELLS_Y		7			// Cells per COLUMN
// Walls
#define NUM_WALLS_X		NUM_CELLS_X-1
#define NUM_WALLS_Y		NUM_CELLS_Y-1

extern const char start_screen_str[];

void GameBoard(void);
void SetNumWalls(int num_walls,uint8_t player);
void SetTime(int time);
void ColorCell(int row_b,int col_b,uint16_t color);
void ColorWall(int row_b,int col_b,uint8_t orientation,uint16_t color);
void WaitScreen(uint16_t bg_color, uint16_t text_color, char* str);
void ErrMsg(uint16_t bg_color, uint16_t text_color, char* str);
//void ColorBackground(int x0,int y0,int x1,int y1,uint8_t orientation, uint16_t color);

#endif
