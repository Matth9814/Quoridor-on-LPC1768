/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_timer.c
** Last modified Date:  2014-09-25
** Last Version:        V1.00
** Descriptions:        functions to manage T0 and T1 interrupts
** Correlated files:    timer.h
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
#include "lpc17xx.h"
#include "timer.h"
#include "../game/game.h"
#include "../game/GUI.h"

/******************************************************************************
** Function name:		Timer0_IRQHandler
**
** Descriptions:		Timer/Counter 0 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/

void TIMER0_IRQHandler (void)
{
	
	DecreaseTurnTime();
  //LPC_TIM0->IR = 1;			// Interrupt cleared in InitTurnTimer
  return;
}


/******************************************************************************
** Function name:		Timer1_IRQHandler
**
** Descriptions:		Timer/Counter 1 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void TIMER1_IRQHandler (void)
{
	switch(timer_mode){
		case VICTORY: // Overwrites the victory screen after a certain interval
			StartScreen();
			break;
		case ERR: // Overwrites error messages after a certain interval
			ErrMsg(COLOR_BG,COLOR_BG, "00000000000000000000000000000");
			break;
		default:
			break;
	}
	// The timer is configured to disable itself when it ends to count
	// The interrupt flag still needs to be cleared
  LPC_TIM1->IR = 1;
  return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
