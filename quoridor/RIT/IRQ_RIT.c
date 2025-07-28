/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_RIT.c
** Last modified Date:  2014-09-25
** Last Version:        V1.00
** Descriptions:        functions to manage T0 and T1 interrupts
** Correlated files:    RIT.h
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
#include "lpc17xx.h"
#include "RIT.h"
#include "../game/game.h"

/******************************************************************************
** Function name:		RIT_IRQHandler
**
** Descriptions:		REPETITIVE INTERRUPT TIMER handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
//#define PROLONGED 20 // ~1 second

void RIT_IRQHandler (void)
{		
	static int select=0, down=0, left=0, right=0, up=0; // To keep joysticks position
	int pins;
	
	if (match == IN_PROGRESS){ // Joystick not active when there is no match
		
		// The simulator has some bugs in the LCD when pressing buttons and joystick too fast
		
		// If the handling of a RIT interrupt is not finished within 50ms the interrupts will
		// accumulate but the handling will not be preempted by them
		
		// Disable interrupts since they may generate problems when handling the movement
		// EINTx have higher priority (1,2,3) than the RIT interrupt (4)
		NVIC_DisableIRQ(EINT0_IRQn);
		NVIC_DisableIRQ(EINT1_IRQn);
		NVIC_DisableIRQ(EINT2_IRQn);
		
		// The FIOPIN register seems to reset the pin values every time it is read
		// I need to read all joystick pins value at once
		pins = LPC_GPIO1->FIOPIN & (0x1F << 25);
		
		
		
		// SELECT
  	if((pins & (1<<25)) == 0){	
			/* Joytick SELECT pressed */
			select++;
			switch(select){
				case 1:
					ConfirmMove();
					break;
				default:
					break;
			}
		}
		else{
				select=0; // Released
		}
		
		// DOWN
		if((pins & (1<<26)) == 0){	
			/* Joytick DOWN pressed */
			down++;
			switch(down){
				case 1:
					Move(DOWN);
					break;
				default:
					break;
			}
		}
		else{
				down=0; // Released
		}
		
		// LEFT
		if((pins & (1<<27)) == 0){	
			/* Joytick LEFT pressed */
			left++;
			switch(left){
				case 1:
					Move(LEFT);
					break;
				default:
					break;
			}
		}
		else{
				left=0; // Released
		}
		
		// RIGHT
		if((pins & (1<<28)) == 0){	
			/* Joytick RIGHT pressed */
			right++;
			switch(right){
				case 1:
					Move(RIGHT);
					break;
				default:
					break;
			}
		}
		else{
				right=0; // Released
		}
		
		// UP
		if((pins & (1<<29)) == 0){	
			/* Joytick UP pressed */
			up++;
			switch(up){
				case 1:
					Move(UP);
					break;
				default:
					break;
			}
		}
		else{
				up=0; // Released
		}
		
		if(match == IN_PROGRESS){ // Do not enable these interrupts if the match is over
			// Clear pending interrupt 
			LPC_SC->EXTINT &= (1 << 0);
			LPC_SC->EXTINT &= (1 << 1);
			LPC_SC->EXTINT &= (1 << 2);
			// Avoids to execute the ISR of interrupt triggered while disabled (pending bit could still be set)
			NVIC_ClearPendingIRQ(EINT0_IRQn);
			NVIC_ClearPendingIRQ(EINT1_IRQn);
			NVIC_ClearPendingIRQ(EINT2_IRQn);
			// Enable again interrupts
			NVIC_EnableIRQ(EINT1_IRQn);  	// Enable mode switch (KEY1)
			// Enable match reset (when match==NO_MATCH it is enabled in Start/VictoryScreen so there is no need to do it here)
			NVIC_EnableIRQ(EINT0_IRQn);
			if(mode == WALL)
				NVIC_EnableIRQ(EINT2_IRQn);		// Enabled ONLY in WALL mode
		}
	}
  LPC_RIT->RICTRL |= 0x1;	/* clear interrupt flag */

	return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
