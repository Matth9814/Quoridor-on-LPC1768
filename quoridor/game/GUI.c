#include <stdio.h>
#include "GUI.h"
#include "../GLCD/GLCD.h"

// Grid
#define BOARD_XLIMIT 	8			// Board side limit pixel
#define BOARD_YLIMIT  8			// Board Top/bottom limit pixel
#define	CELL_SIDE			26		// Cell side dimension (pixels) 
#define	WALL_WIDTH		5			// Wall width dimension (pixels) 
// Info panel
#define INFO_XLIMIT 	7			// Info side limit
#define INFO_YLIMIT		20		// Info top limit w.r.t. the board
#define INFO_PANEL_X 	70		// Info panel x dimension (pixels)
#define INFO_PANEL_Y 	50		// Info panel y dimension (pixels)
#define	NUM_INFO			3			// Player1 walls | Time | Player2 Walls	
// Orientation
#define VERTICAL 			0
#define HORIZONTAL		1

static void ColorBackground(int x0,int y0,int x1,int y1,uint8_t orientation, uint16_t color);

void GameBoard(void){
	int i,j;
	int row, col;
	
	WaitScreen(COLOR_BG, COLOR_BG, (char*) start_screen_str); // Just overwrite the start screen text
	
	// Create board background
	ColorBackground(BOARD_XLIMIT, BOARD_YLIMIT, MAX_X-BOARD_XLIMIT-1, MAX_X-BOARD_YLIMIT-1, HORIZONTAL, COLOR_BOARD);
	/*for(row=0;row<MAX_Y;row++){
		if(row > BOARD_XLIMIT && row < MAX_X-BOARD_XLIMIT-1)
			LCD_DrawLine(BOARD_XLIMIT+1, row, MAX_X-BOARD_XLIMIT-2, row, COLOR_BOARD);
		}*/
	
	// Board background limit
	LCD_DrawLine(BOARD_XLIMIT, BOARD_YLIMIT, MAX_X-BOARD_XLIMIT-1, BOARD_YLIMIT, COLOR_GRID);
	LCD_DrawLine(BOARD_XLIMIT, BOARD_YLIMIT, BOARD_XLIMIT, MAX_X-BOARD_YLIMIT-1, COLOR_GRID);
	LCD_DrawLine(MAX_X-BOARD_XLIMIT-1, MAX_X-BOARD_YLIMIT-1, MAX_X-BOARD_XLIMIT-1, BOARD_YLIMIT, COLOR_GRID);
	LCD_DrawLine(MAX_X-BOARD_XLIMIT-1, MAX_X-BOARD_YLIMIT-1, BOARD_XLIMIT, MAX_X-BOARD_YLIMIT-1, COLOR_GRID);
	
	// Create Grid
	// (row, cell) = cell coordinate in the cell grid
	// (i, j) = pixel top-left coordinate of a cell
	j = BOARD_XLIMIT+WALL_WIDTH+1;
	for (row=0; row<NUM_CELLS_X; row++){
		i=BOARD_XLIMIT+WALL_WIDTH+1;
			for(col=0; col<NUM_CELLS_Y; col++){
				LCD_DrawLine(i, j, i, j+CELL_SIDE-1, COLOR_GRID);
				LCD_DrawLine(i, j, i+CELL_SIDE-1, j, COLOR_GRID);
				LCD_DrawLine(i, j+CELL_SIDE-1, i+CELL_SIDE-1, j+CELL_SIDE-1, COLOR_GRID);
				LCD_DrawLine(i+CELL_SIDE-1, j, i+CELL_SIDE-1, j+CELL_SIDE-1, COLOR_GRID);
				i+=CELL_SIDE+WALL_WIDTH;
			}
		j+=CELL_SIDE+WALL_WIDTH;
	}
	
	// Info board
	i = MAX_X+INFO_YLIMIT;
	j = INFO_XLIMIT+1;
	for(col=0; col<NUM_INFO; col++){
		ColorBackground(j, i, j+INFO_PANEL_X-1, i+INFO_PANEL_Y-1, HORIZONTAL, COLOR_BOARD);
		/*for(row=1; row<INFO_PANEL_Y-1; row++)
			LCD_DrawLine(j+1, i+row, j+INFO_PANEL_X-2, i+row, COLOR_BOARD);*/
		LCD_DrawLine(j, i, j+INFO_PANEL_X-1, i, COLOR_GRID);
		LCD_DrawLine(j, i, j, i+INFO_PANEL_Y-1, COLOR_GRID);
		LCD_DrawLine(j+INFO_PANEL_X-1, i, j+INFO_PANEL_X-1, i+INFO_PANEL_Y-1, COLOR_GRID);
		LCD_DrawLine(j, i+INFO_PANEL_Y-1, j+INFO_PANEL_X-1, i+INFO_PANEL_Y-1, COLOR_GRID);
		j += INFO_XLIMIT+INFO_PANEL_X;
	}
	GUI_Text(INFO_XLIMIT+4,MAX_X+INFO_YLIMIT+4,(uint8_t*) "P1 walls",COLOR_GRID, COLOR_BOARD);
	GUI_Text(INFO_PANEL_X+INFO_XLIMIT*2+20,MAX_X+INFO_YLIMIT+4,(uint8_t*) "Time",COLOR_GRID, COLOR_BOARD);
	GUI_Text(INFO_PANEL_X*2+INFO_XLIMIT*3+4,MAX_X+INFO_YLIMIT+4,(uint8_t*) "P2 walls",COLOR_GRID, COLOR_BOARD);
}

void SetNumWalls(int num_walls, uint8_t player){
	int mult = (player==1)? 2:0;
	int x = INFO_PANEL_X*mult+INFO_XLIMIT*(mult+1);
	char num[2];
	int digit = sprintf(num,"%d",num_walls);
	GUI_Text(x+(INFO_PANEL_X-3*8)/2,MAX_X+INFO_YLIMIT+25,(uint8_t*) "000",COLOR_BOARD, COLOR_BOARD); // Clear previous text
	GUI_Text(x+(INFO_PANEL_X-digit*8)/2,MAX_X+INFO_YLIMIT+25,(uint8_t*) num,COLOR_GRID, COLOR_BOARD);
}

void SetTime(int time){ // time in seconds
	char num[2];
	int digit = sprintf(num,"%d",time);
	int horiz_offset = INFO_PANEL_X+INFO_XLIMIT*2;
	
	GUI_Text(horiz_offset+(INFO_PANEL_X-3*8)/2,MAX_X+INFO_YLIMIT+25,(uint8_t*) "000",COLOR_BOARD, COLOR_BOARD); // Clear previous text
	GUI_Text(horiz_offset+(INFO_PANEL_X-digit*8)/2,MAX_X+INFO_YLIMIT+25,(uint8_t*) num,COLOR_GRID, COLOR_BOARD);
}	

void ColorCell(int row_b, int col_b, uint16_t color){
	// x and y are the coordinates in the cells NUM_CELLS_X*NUM_CELLS_Y grid
	int row, col;
	
	if(row_b<0 || row_b>NUM_CELLS_X-1 || col_b<0 || col_b>NUM_CELLS_Y-1)
		return;
	row = BOARD_YLIMIT+WALL_WIDTH+1+row_b*(WALL_WIDTH+CELL_SIDE);
	col = BOARD_XLIMIT+WALL_WIDTH+1+col_b*(WALL_WIDTH+CELL_SIDE); // top-left pixel of block (x,y)
	
	ColorBackground(col, row, col+CELL_SIDE-1, row+CELL_SIDE-1, HORIZONTAL, color);
	/*for(i=1; i<CELL_SIDE-1; i++)
		LCD_DrawLine(col+1, row+i, col+CELL_SIDE-2, row+i, color);*/
}

void ColorWall(int row_b, int col_b, uint8_t orientation, uint16_t color){
	// x and y are the coordinate of the center of the wall in the NUM_WALLS_X*NUM_WALLS_Y grid
	int row, col;
	
	// Check wall data structure to understand these limits
	if(row_b<1 || row_b>NUM_CELLS_X || col_b<1 || col_b>NUM_CELLS_Y)
		return;
	
	row = BOARD_YLIMIT+WALL_WIDTH+CELL_SIDE+1+(row_b-1)*(WALL_WIDTH+CELL_SIDE);
	col = BOARD_XLIMIT+WALL_WIDTH+CELL_SIDE+1+(col_b-1)*(WALL_WIDTH+CELL_SIDE); // top-left pixel of the center of the wall (x,y)
	
	if (orientation){ // HORIZONTAL
		col -= CELL_SIDE;
		ColorBackground(col-1, row-1, col+2*CELL_SIDE+WALL_WIDTH, row+WALL_WIDTH, HORIZONTAL, color);
		/*for(i=0; i<WALL_WIDTH; i++)
			LCD_DrawLine(col, row+i, col+2*CELL_SIDE+WALL_WIDTH-1, row+i, color);*/
	}
	else{ // VERTICAL
		row -= CELL_SIDE;
		ColorBackground(col-1, row-1, col+WALL_WIDTH, row+2*CELL_SIDE+WALL_WIDTH, VERTICAL, color);
		// Equivalent result by generating the lines horizontally
		//ColorBackground(col-1, row-1, col+WALL_WIDTH, row+2*CELL_SIDE+WALL_WIDTH, HORIZONTAL, color); 
		/*for(i=0; i<WALL_WIDTH; i++)
			LCD_DrawLine(col+i, row, col+i, row+2*CELL_SIDE+WALL_WIDTH-1, color);
	*/}
}

static void ColorBackground(int x0, int y0, int x1, int y1, uint8_t orientation, uint16_t color){
	// (x0+1,y0+1) top-left point of the panel
	// (x1-1,y1-1) bottom-left point of the panel
	int row, tmp;
	
	if (orientation == HORIZONTAL){
		if(y0>y1){ // The LCD_Drawline does the same for x0 and x1
			tmp=y0;
			y0=y1;
			y1=tmp;
		}
		for(row=y0+1;row<y1;row++)
			LCD_DrawLine(x0+1, row, x1-1, row, color);
	}
	else{ // VERTICAL (HORIZONTAL right-turned of 90°)
		// Can be used to generate the lines vertically instead of horizontally
		if(x0>x1){ // The LCD_Drawline does the same for y0 and y1
			tmp=x0;
			x0=x1;
			x1=tmp;
		}
		for(row=x0+1;row<x1; row++)
			LCD_DrawLine(row, y0+1, row, y1-1, color);
	}
}

void WaitScreen(uint16_t bg_color, uint16_t text_color, char* str){
	int digit, horiz_offset;
	int vert_offset = (MAX_Y-16)/2;
	
	// Str length
	for (digit=0;str[digit]!=NULL;digit++);
	horiz_offset = (MAX_X-(digit*8))/2; // 8 is defined in GLCD.c as the char x length in pixels
	
	LCD_Clear(COLOR_BG);
	GUI_Text(horiz_offset, vert_offset,(uint8_t*) str, text_color, bg_color);
}

void ErrMsg(uint16_t bg_color, uint16_t text_color, char* str){
	int digit;
	
	// Overwrite previous text
	GUI_Text(BOARD_XLIMIT, MAX_X+(MAX_Y-2*BOARD_XLIMIT-MAX_X-INFO_PANEL_Y-INFO_YLIMIT)/2,(uint8_t*) "00000000000000000000000000000", COLOR_BG,COLOR_BG);
	// Str length
	for (digit=0;str[digit]!=NULL;digit++);
	GUI_Text(BOARD_XLIMIT, MAX_X+(MAX_Y-2*BOARD_XLIMIT-MAX_X-INFO_PANEL_Y-INFO_YLIMIT)/2,(uint8_t*) str, text_color, bg_color);
}
