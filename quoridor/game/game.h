#ifndef __GAME_H
#define __GAME_H

#include <stdint.h>
#define TURN_TIMER			0	 // Change the interrupt flag cleared if timer changes
#define UTILITY_TIMER		1	 // Used to time the victory screen and error message deletion

typedef enum {DOWN, LEFT, RIGHT, UP, NUM_DIR, NO_MOVE} dir_t; 	// To specify the direction to move
typedef enum {NO_MATCH, IN_PROGRESS} match_t;								// Match states
typedef enum {PLAYER, WALL, NUM_MODE} mode_t;								// Current turn mode
typedef enum {VICTORY, ERR} utility_t;

//extern volatile match_t match;			// Match status
extern volatile match_t match;				// Match status
extern volatile mode_t mode;					// Current active mode
extern volatile utility_t timer_mode; // Utility timer (timer1) mode

void SetupGame(void);
void StartScreen(void);
void DecreaseTurnTime(void);
void Move(dir_t dir);
void ConfirmMove(void);
void ChangeMode(mode_t new_mode);
void RotateWall(void);

#endif
