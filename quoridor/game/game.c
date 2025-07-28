#include <stdio.h>
#include "GUI.h"
#include "player.h"
#include "game.h"
#include "lpc17xx.h"
#include "../timer/timer.h"
#include "../button_EXINT/button.h"
#include "../RIT/RIT.h"

#define NUM_WALLS_P			8
#define MOVE_TIME				20 // seconds
#define ERROR_INTERVAL  0x47868C0 // Permanence time of errors on screen (3s)

typedef enum {FALSE, TRUE} bool_t;
typedef enum{NOT_VISITED, VISITED} cell_state_t;
typedef enum {LOW, HIGH, NUM_LIMIT} limit_t; 
typedef enum {VERTICAL, HORIZONTAL, NUM_ORIENT, NO_WALL} wall_states_t;			// Wall positions
typedef struct move_record{
	uint8_t p_id;
	uint8_t wp_o; // 4 bits player/wall mode + 4 bits wall orientation
	uint8_t x;
	uint8_t y;
} move_rec_t;
typedef struct possible_player_move{ // The direction is given by the position in the move array
	pos_t pos;
	bool_t isPossible;
} move_t;
typedef struct wall_board{ // Wall placed on the board
	wall_states_t state; // orientation + no_wall
	uint8_t player;
} wallb_t;
typedef struct possible_wall_move{ // Wall to move in wall mode
	pos_t pos;
	wall_states_t state; // orientation
} wall_t;
typedef struct FIFO{ // Implemented as a circular buffer (not really needed due to the sizing)
	int head;
	int tail;
	pos_t queue[NUM_CELLS_X*NUM_CELLS_Y];
	// Sized with the worst case just to be safe 
	// It is actually impossible since at least one cell is occupied by the player that is moving
} buff_t;

static void InitTurnTimer(void);
static void canJump(move_t* curr_pos, pos_t opp_pos, dir_t dir);
static void IsMovePossible(move_t* curr_pos,dir_t dir,uint8_t p_id);
static void ResetMove(uint8_t p_id);
static void ChangeTurn(void);
static bool_t isVictory(void);
static void EndMatch(void);
static void PossibleMoves(void);
static void HighlightPossibleMoves(uint16_t color,dir_t dont_touch);
static void RestoreCrossWallGUI(void);
static void VictoryScreen(void);
static bool_t isThereWall(pos_t pos,dir_t dir);
static bool_t OutOfBoard(pos_t pos, pos_t board_low_limits, pos_t board_up_limits);
static void InitUtilityTimer(utility_t tmode, uint32_t time_interval);
// BFS
static void InitializeBFS(void);
static void enqueue(pos_t pos);
static pos_t dequeue(void);
static bool_t isEmpty(void);
static bool_t isTrap(uint8_t p_id,bool_t softlock_check);

// TO WIN A PLAYER HAS TO REACH THE OPPOSITE SIDE OF THE BOARD
const pos_t mov_offset[NUM_DIR] = {{1,0},{0,-1},{0,1},{-1,0}}; 	// DOWN, LEFT, RIGHT, UP
const pos_t start_pos[NUM_PLAYERS] = {{6,3},{0,3}};							// Players starting position
const uint16_t color_p[NUM_PLAYERS] = {Beige, LightOrange};			// Players color
const pos_t start_wall = {3,3};																	// Wall start position
const pos_t wall_check_offset[NUM_DIR][2] = 
{{{1,1}, {1,0}}, // DOWN
{{0,0},{1,0}}, 	 // LEFT
{{1,1},{0,1}},	 // RIGHT
{{0,0},{0,1}}};	 // UP
const char start_screen_str[] = "Press INT0 to start!!";

volatile player_t p[NUM_PLAYERS];	// Players array
volatile pos_t cellb_limit[NUM_LIMIT] = {{0,0},{NUM_CELLS_X,NUM_CELLS_Y}};			// Cells board limits
volatile pos_t wallb_limit[NUM_LIMIT] = {{1,1},{NUM_CELLS_X,NUM_CELLS_Y}};			// Walls board limits
volatile cell_state_t cells_board[NUM_CELLS_X][NUM_CELLS_Y];			// Cells board matrix
volatile wallb_t walls_board[NUM_CELLS_X+1][NUM_CELLS_Y+1];	// Walls board matrix
// There are fake walls on the wall board perimeter to ease the wall check : BE CAREFUL TO INDEXES
volatile buff_t to_visit; // Data structure for BFS


volatile match_t match = NO_MATCH;				// Match status
volatile uint8_t turn_time = MOVE_TIME;		// Time left in this turn
volatile move_rec_t move;									// Current move info
volatile move_t possible_moves[NUM_DIR];	// Possible moves in the NUM_DIRS directions
volatile dir_t move_dir;									// Player move direction of a turn
volatile wall_t curr_wall;								// Current position of the wall
volatile mode_t mode;											// Current active mode
volatile utility_t timer_mode;						// Mode of the utility timer (timer1)

/* SPACE ORGANIZATION
	 _ _ _ _> y = columns
	|_|_|_|_|
	|_|_|_|_|
	|_|_|_|_|
	|_|_|_|_|
	v
	x = rows
*/

void SetupGame(void){
	int i,j;

	// Initialize cells board data structure
	/*for(i=0;i<NUM_CELLS_X;i++)
		for(j=0;j<NUM_CELLS_Y;j++)
			cells_board[i][j] = 0;
	*/
	
	// Initialize walls board data structure
	for(i=0;i<=NUM_CELLS_X;i++)
		for(j=0;j<=NUM_CELLS_Y;j++){
			walls_board[i][j].state = NO_WALL;
			walls_board[i][j].player = NO_PLAYER;
		}
		
	// Player setup
	p[P1] = Player(P1, NUM_WALLS_P, color_p[P1], start_pos[P1]);
	p[P2] = Player(P2, NUM_WALLS_P, color_p[P2], start_pos[P2]);
	
	// Move initialization
	ResetMove(P2); // P1 starts
	
	GameBoard(); // Create board and info panels GUI
	ColorCell(p[P2].pos.row, p[P2].pos.col, color_p[P2]); // P2 token
	ColorCell(p[P1].pos.row, p[P1].pos.col, color_p[P1]); // P1 token
	
	// Setup info panels
	SetNumWalls(NUM_WALLS_P,P1);
	SetNumWalls(NUM_WALLS_P,P2);
	SetTime(MOVE_TIME);
	
	// Flag match start
	match = IN_PROGRESS;
	
	// First turn
	move_dir = NO_MOVE; // Needed by possible moves highlight removal
	ChangeTurn();
}

void StartScreen(void){
	WaitScreen(COLOR_BG, COLOR_TEXT,(char*) start_screen_str);
	NVIC_EnableIRQ(EINT0_IRQn);				// Enable EINT0
}

static void VictoryScreen(void){
	char str[] = "P1 wins!!!";
	sprintf(str,"P%d wins!!!",move.p_id+1);
	WaitScreen(COLOR_BG, COLOR_TEXT,str);
	InitUtilityTimer(VICTORY,0x2FAF080);
}

static void InitUtilityTimer(utility_t tmode, uint32_t time_interval){
	timer_mode = tmode; // Set function to call when the count ends
	disable_timer(UTILITY_TIMER); // In case it is already counting reset the timer
	init_timer(UTILITY_TIMER,time_interval);
	LPC_TIM1->IR = 1;											// Clear timer1 interrupt flag
	enable_timer(UTILITY_TIMER);
}

static void InitTurnTimer(void){
	//reset_timer(TURN_TIMER);
	init_timer(TURN_TIMER,0x17D7840);			// 1 second period
	LPC_TIM0->IR = 1;											// Clear timer0 interrupt flag
	enable_timer(TURN_TIMER);
}

void DecreaseTurnTime(void){
	turn_time--;
	
	if(turn_time == 0){
		// Mark move as not valid
		move.wp_o = 0x01;
		ChangeTurn();			// Change turn 
	}
	else{
		InitTurnTimer();	// Init the timer for this turn
		SetTime(turn_time);
	}
}

void ChangeTurn(void){
	uint8_t new_p;
	
	// Stop timer
	// The timer disable itself when it finishes to count with the current settings
	//disable_timer(TURN_TIMER); 
	// Reset turn time
	turn_time = MOVE_TIME;
	// Reset move recorder
	// 0000_000P_0000_0000_0000_0000_0000_0000
	new_p = (move.p_id+1)%NUM_PLAYERS;
	ResetMove(new_p);
	
	// Set default turn mode
	if(mode == PLAYER)
		HighlightPossibleMoves(COLOR_BOARD, move_dir); // Remove previously highlighted possible moves
	ChangeMode(PLAYER);
	
	// Reset time
	SetTime(turn_time);
	
	// Check if one of the two players is softlocked 
	if(isTrap(P1,TRUE) == TRUE || isTrap(P2, TRUE) == TRUE){
		ErrMsg(COLOR_BG, COLOR_TEXT, "Softlock? Really? >:(");
		InitUtilityTimer(ERR,ERROR_INTERVAL);
	}
	
	InitTurnTimer();
}

void PossibleMoves(void){
	pos_t pos_p = p[move.p_id].pos;
	dir_t dir;
	// Player already changed in move record variable
	for(dir=DOWN; dir<NUM_DIR; dir++){
		// Compute possible move position
		possible_moves[dir].pos.row = pos_p.row;
		possible_moves[dir].pos.col = pos_p.col;
		IsMovePossible((move_t*)&possible_moves[dir],dir,move.p_id); // Check if the move is possible and update the struct
	}
}

static void ResetMove(uint8_t p_id){
	move.p_id = p_id; 
	move.wp_o = 0; 	// Player mode 
	move.x = 0;			// row
	move.y = 0;			// col
}

void Move(dir_t dir){
	pos_t tmp_pos;
	
	// Check that the move is possible
	if(mode == PLAYER && possible_moves[dir].isPossible == TRUE){
		move_dir = dir;
	}
	else if(mode == WALL){ 
		// Compute position to move
		tmp_pos.row = curr_wall.pos.row + mov_offset[dir].row;
		tmp_pos.col = curr_wall.pos.col + mov_offset[dir].col;
		if(OutOfBoard(tmp_pos, wallb_limit[LOW], wallb_limit[HIGH]) == FALSE){ // Remain in the same position if move is not legal 
			// The check on the intersection is done when trying to confirm the move
			// Restore GUI before moving
			RestoreCrossWallGUI();
			// Update wall position
			curr_wall.pos.row = tmp_pos.row;
			curr_wall.pos.col = tmp_pos.col;
			ColorWall(curr_wall.pos.row, curr_wall.pos.col,curr_wall.state,color_p[move.p_id]);
		}
	}
	else{
		// Player move not possible
		ErrMsg(COLOR_BG, color_p[move.p_id], "Move not possible");
		InitUtilityTimer(ERR,ERROR_INTERVAL);
	}
	// The player position is updated only when the move is confirmed
}

void ConfirmMove(void){
	player_t *pl;
	wallb_t *bwall;
	
	if(mode == PLAYER){
		if(move_dir != NO_MOVE){ // Possible only if a valid move has been selected
			pl = (player_t*) &(p[move.p_id]);
			// Update player position
			ColorCell(pl->pos.row, pl->pos.col, COLOR_BOARD); // Remove previous position
			pl->pos.row = move.x = possible_moves[move_dir].pos.row;
			pl->pos.col = move.y = possible_moves[move_dir].pos.col;
			ColorCell(pl->pos.row,pl->pos.col, p[move.p_id].color); // Move token
			// Check victory (only if a player moves)
			if(isVictory() == TRUE){
				EndMatch();
				// Button interrupts are currently disabled and timer0 (not its interrupt handling) is also disabled
				VictoryScreen();
				return;
			}
		}
	}
	else{ // WALL
		
		if(walls_board[curr_wall.pos.row][curr_wall.pos.col].state != NO_WALL || // Check walls in the same position
			(curr_wall.state == HORIZONTAL &&	// Check for wall intersection with other walls edges
				(walls_board[curr_wall.pos.row][curr_wall.pos.col-1].state == HORIZONTAL ||
				walls_board[curr_wall.pos.row][curr_wall.pos.col+1].state == HORIZONTAL)) ||
			(curr_wall.state == VERTICAL &&
				(walls_board[curr_wall.pos.row-1][curr_wall.pos.col].state == VERTICAL ||
				walls_board[curr_wall.pos.row+1][curr_wall.pos.col].state == VERTICAL)) ||
			isTrap(P1,FALSE) == TRUE || isTrap(P2,FALSE) == TRUE){ // Check that both players can still reach the goal after placing this wall
	
			ErrMsg(COLOR_BG, color_p[move.p_id], "Invalid wall position");
			InitUtilityTimer(ERR,ERROR_INTERVAL);
			return; // Invalid move so do not change turn
		}
		pl = (player_t*) &(p[move.p_id]);
		bwall = (wallb_t*) &walls_board[curr_wall.pos.row][curr_wall.pos.col];
		// Record current wall on walls board
		bwall->player = move.p_id;
		bwall->state = curr_wall.state;
		// Decrease player walls
		pl->num_walls--;
		SetNumWalls(pl->num_walls, pl->id);
		// Update move record
		move.x = curr_wall.pos.row;
		move.y = curr_wall.pos.col;
		move.wp_o |= curr_wall.state; // Just add the orientation
	}
	
	// Change turn
	ChangeTurn();
}

static void IsMovePossible(move_t* curr_pos, dir_t dir, uint8_t p_id){
	pos_t opp_pos, move_pos;
	
	opp_pos = p[(p_id+1)%NUM_PLAYERS].pos; // Opposite player position
	curr_pos->isPossible = TRUE; // Mark move as possible (for now)
	// Compute move position
	move_pos.row = curr_pos->pos.row + mov_offset[dir].row;
	move_pos.col = curr_pos->pos.col + mov_offset[dir].col;
	if (OutOfBoard(move_pos, cellb_limit[LOW], cellb_limit[HIGH]) == TRUE) // Board out of bound
		curr_pos->isPossible = FALSE; // Move not possible
	else if (isThereWall(curr_pos->pos,dir) == TRUE) // Wall blocking
		curr_pos->isPossible = FALSE;
	else if (move_pos.row == opp_pos.row && move_pos.col == opp_pos.col){	// On the opposite player
		// move_poss->isPossible and pos are set inside canJump 
		canJump(curr_pos,opp_pos,dir); // Check if the move needs to jump the opponent and update the struct in case it can
	}
	else{ // Update possible position if there is no problem
		curr_pos->pos.row = move_pos.row;
		curr_pos->pos.col = move_pos.col;
	}
}

static void HighlightPossibleMoves(uint16_t color, dir_t dont_touch){
	dir_t dir;
	for (dir=DOWN;dir<NUM_DIR;dir++){
		if(dir == dont_touch || possible_moves[dir].isPossible == FALSE)
				continue;
		ColorCell(possible_moves[dir].pos.row,possible_moves[dir].pos.col, color);
	}
}

static bool_t isVictory(void){
	pos_t v_pos = start_pos[(move.p_id+1)%NUM_PLAYERS];
	if(move.x == v_pos.row){ // Victory
		return TRUE;
	}
	return FALSE;
}

static void EndMatch(void){
	match = NO_MATCH; // Match end (disables joystick)
	// Stop timer;
	disable_timer(TURN_TIMER);
	// Disable KEY1 and KEY2
	NVIC_DisableIRQ(EINT1_IRQn);
	NVIC_DisableIRQ(EINT2_IRQn);
	NVIC_DisableIRQ(EINT0_IRQn);	// EINT0 is disabled during wait screen generation
}

static void canJump(move_t* curr_pos, pos_t opp_pos, dir_t dir){
	pos_t jump_pos;
	jump_pos.row = opp_pos.row + mov_offset[dir].row;
	jump_pos.col = opp_pos.col + mov_offset[dir].col;
	 // Check Out of Bound and wall behind the opponent
	if(OutOfBoard(jump_pos, cellb_limit[LOW], cellb_limit[HIGH]) == FALSE && isThereWall(opp_pos,dir) == FALSE){
		// Update possible move with jump info
		curr_pos->pos.row = jump_pos.row;
		curr_pos->pos.col = jump_pos.col;
		//curr_pos->isPossible = TRUE;
		return;
	}
	curr_pos->isPossible = FALSE; // There is an out of bound
}

bool_t OutOfBoard(pos_t pos, pos_t board_low_limits, pos_t board_high_limits){
	if (pos.row < board_low_limits.row || pos.row >= board_high_limits.row || pos.col < board_low_limits.col || pos.col >= board_high_limits.col) // Out of bound check
		return TRUE;
	return FALSE;
}

void ChangeMode(mode_t new_mode){
	// Setup mode
	if(new_mode == PLAYER){
		if(mode == WALL) // Restore GUI when changing from WALL mode
			RestoreCrossWallGUI();
		// Update mode
		mode = new_mode;
		// Update move recorder
		move.wp_o = (mode << 4); // 0001_0000 WALL mode HORIZONTAL | 0000_0000 PLAYER mode
		// Compute and highlight possible moves
		PossibleMoves();
		HighlightPossibleMoves(COLOR_GRID, NO_MOVE);
		// Reset move direction (do not place before possible moves highlight removal)
		move_dir = NO_MOVE;
	}
	else if(p[move.p_id].num_walls > 0){ // WALL mode if the player has walls
		// Update mode
		mode = new_mode;
		// Update move recorder
		move.wp_o = (mode << 4);
		// Remove possible moves highlight
		HighlightPossibleMoves(COLOR_BOARD, move_dir);
		// Reset wall position variable
		curr_wall.state = HORIZONTAL;
		curr_wall.pos = start_wall;
		ColorWall(start_wall.row, start_wall.col, HORIZONTAL, color_p[move.p_id]);
	}
	else{
		ErrMsg(COLOR_BG, color_p[move.p_id], "Walls ended, move the token");
		InitUtilityTimer(ERR,ERROR_INTERVAL);
	}
}

void RotateWall(void){
	// Restore GUI
	RestoreCrossWallGUI();
	// Rotate wall and update GUI
	curr_wall.state = (curr_wall.state+1)%NUM_ORIENT;
	ColorWall(curr_wall.pos.row, curr_wall.pos.col, curr_wall.state, color_p[move.p_id]);
}

bool_t isThereWall(pos_t pos, dir_t dir){
	/*
	Graphical representation of player and wall boards in the same space
	
		0 0 1 1 2 2 3 3 4
	0 W   W   W   W   W
	0   P   P   P   P   
	1 W   W   W   W   W
	1   P   O   P   P   
	2 W   W   W   W   W
	2   P   P   P   P   
	3 W   W   W   W   W
	
	The row/columns marked as x are present in walls_board to ease the wall check but
	no wall can be placed there
	WALL POSITIONS to check
	DOWN	(1, 0): HORIZONTAL 	(pos+1,pos+1)	or (pos+1,pos)
	LEFT 	(0,-1): VERTICAL 		(pos,pos) 		or (pos+1,pos)
	RIGHT	(0, 1): VERTICAL 		(pos+1,pos+1)	or (pos,pos+1)
	UP		(-1,0): HORIZONTAL 	(pos,pos) 		or (pos,pos+1)
	*/
	const pos_t *w_off = wall_check_offset[dir];
	wall_states_t orient = (dir==DOWN || dir==UP)? HORIZONTAL:VERTICAL;
	
	if(walls_board[pos.row+w_off[0].row][pos.col+w_off[0].col].state == orient ||
		walls_board[pos.row+w_off[1].row][pos.col+w_off[1].col].state == orient)
		return TRUE;
	
	return FALSE; // No wall in the way
}

static void RestoreCrossWallGUI(void){
	wallb_t bwall;
	uint8_t row_offset, col_offset;
	// Restore GUI before moving
	// Check #1: postion the wall is on
	bwall = walls_board[curr_wall.pos.row][curr_wall.pos.col];
	if(bwall.state == NO_WALL)
		// Just paint the board background if there is no player wall
		ColorWall(curr_wall.pos.row, curr_wall.pos.col,curr_wall.state,COLOR_BOARD);
	else{
		ColorWall(curr_wall.pos.row, curr_wall.pos.col,curr_wall.state,COLOR_BOARD);
		ColorWall(curr_wall.pos.row, curr_wall.pos.col,bwall.state,color_p[bwall.player]);
	}
	// Check #2: left/right if it is horizontal and up/down if it is vertical
	row_offset = (curr_wall.state==VERTICAL)? 1:0;
	col_offset = (curr_wall.state==HORIZONTAL)? 1:0;
	// No need to check if the control is out of bound because it can't be
	// There is the ring of fake walls set to NO_WALL and the wall is not allowed to move there
	bwall = walls_board[curr_wall.pos.row-row_offset][curr_wall.pos.col-col_offset];
	if(bwall.state == curr_wall.state) // Check up/left wall (vertical/horizontal)
		ColorWall(curr_wall.pos.row-row_offset, curr_wall.pos.col-col_offset, bwall.state, color_p[bwall.player]);
	bwall = walls_board[curr_wall.pos.row+row_offset][curr_wall.pos.col+col_offset];
	if(bwall.state == curr_wall.state) // Check down/right wall (vertical/horizontal)
		ColorWall(curr_wall.pos.row+row_offset, curr_wall.pos.col+col_offset, bwall.state, color_p[bwall.player]);
}

// BFS
static void InitializeBFS(void){
	int i,j;
	
	// Initialize cells board data structure
	for(i=0;i<NUM_CELLS_X;i++)
		for(j=0;j<NUM_CELLS_Y;j++)
			cells_board[i][j] = NOT_VISITED;
	
	// Initialize node queue to invalid positions
	for(i=0;i<NUM_CELLS_X*NUM_CELLS_Y;i++){
		to_visit.queue[i].row = -1;
		to_visit.queue[i].col = -1;
	}
}

static bool_t isEmpty(void){
	if(to_visit.head == to_visit.tail)
		return TRUE;
	return FALSE;
}

static void enqueue(pos_t pos){
	int new_tail = (to_visit.tail+1)%(NUM_CELLS_X*NUM_CELLS_Y);
	if(new_tail == to_visit.head)
		// This should never happen due to the sizing of the buffer
		// It would mean that the buffer is full (except for the cell pointed by to_visit.tail)
		return;
	
	to_visit.queue[to_visit.tail].row = pos.row;
	to_visit.queue[to_visit.tail].col = pos.col;
	to_visit.tail = new_tail;
}

static pos_t dequeue(void){
	pos_t pos = {-1,-1};
	if(isEmpty() == TRUE)
		return pos; // Return invalid position
	
	pos.row = to_visit.queue[to_visit.head].row;
	pos.col = to_visit.queue[to_visit.head].col;
	to_visit.head = (to_visit.head+1)%(NUM_CELLS_X*NUM_CELLS_Y);
	return pos;
}

static bool_t isTrap(uint8_t p_id, bool_t softlock_check){ // Breadth-First Search algorithm
	pos_t curr_pos;
	dir_t dir;
	move_t curr_move;
	uint8_t opp_p_id = (p_id+1)%NUM_PLAYERS; // Opponent
	
	// Temporary place the wall to do the check
	/* We do not need to check the previous state of the wall because we are already sure there
		 is no other wall in this position or in adjacent positions at this point (so this position state is NO_WALL) */
	if(softlock_check == FALSE)
		walls_board[curr_wall.pos.row][curr_wall.pos.col].state = curr_wall.state;
	
	InitializeBFS();
	to_visit.head = 0; // Head points to first valid position
	// Insert player position
	to_visit.queue[to_visit.head].row = p[p_id].pos.row;
	to_visit.queue[to_visit.head].col = p[p_id].pos.col;
	cells_board[p[p_id].pos.row][p[p_id].pos.col] = VISITED; // Mark starting position as visited
	to_visit.tail = 1; // Tail points to first free position
	
	// Mark opposite player cell since we cannot go on him, just over
	cells_board[p[opp_p_id].pos.row][p[opp_p_id].pos.col] = VISITED;
    
	while (!isEmpty()){
		// Extract cell
		curr_pos = dequeue();
		
		for(dir=DOWN; dir<NUM_DIR; dir++){ // Add at most cells since the player has at most 4 directions
			curr_move.pos.row = curr_pos.row;
			curr_move.pos.col = curr_pos.col;
			IsMovePossible(&curr_move,dir,p_id); // Check if the move is possible and update the struct
			// Skip the node if it is unreachable (illegal move) or it has already been added to the queue
			if(curr_move.isPossible == TRUE && cells_board[curr_move.pos.row][curr_move.pos.col] == NOT_VISITED){
				if(curr_move.pos.row == start_pos[opp_p_id].row){ // Win condition reached
					// Restore wall state
					if(softlock_check == FALSE)
						walls_board[curr_wall.pos.row][curr_wall.pos.col].state = NO_WALL;
					return FALSE; // There is still a way to win
				}
				cells_board[curr_move.pos.row][curr_move.pos.col] = VISITED;
				enqueue(curr_move.pos);
			}
		}
	}
	
	// Restore wall state
	if(softlock_check == FALSE)
		walls_board[curr_wall.pos.row][curr_wall.pos.col].state = NO_WALL;
		
	return TRUE;
}
