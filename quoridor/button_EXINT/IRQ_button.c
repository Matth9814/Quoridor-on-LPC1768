#include "button.h"
#include "lpc17xx.h"
#include "../game/game.h"
#include "../timer/timer.h"

void EINT0_IRQHandler (void)	  	// INT0 (Debouncing is managed)
{	
	disable_timer(TURN_TIMER);
	disable_timer(UTILITY_TIMER);
	NVIC_DisableIRQ(EINT0_IRQn);
	NVIC_DisableIRQ(EINT1_IRQn);
	NVIC_DisableIRQ(EINT2_IRQn);
	
	SetupGame();
	
	// Clear pending interrupt 
	LPC_SC->EXTINT &= (1 << 0);
	LPC_SC->EXTINT &= (1 << 1);
	//LPC_SC->EXTINT &= (1 << 2);
	// Avoids to execute the ISR of interrupt triggered while disabled (pending bit could still be set)
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_ClearPendingIRQ(EINT1_IRQn);
	//NVIC_ClearPendingIRQ(EINT2_IRQn);
	// Enables again interrupt
	NVIC_EnableIRQ(EINT1_IRQn);  	// Enable mode switch (KEY1)
	NVIC_EnableIRQ(EINT0_IRQn);		// Enable match reset
	//NVIC_EnableIRQ(EINT2_IRQn);		// Enabled by WALL mode in EINT1_IRQHandler
}


void EINT1_IRQHandler (void)	  	// KEY1	active WALL mode
{
	NVIC_DisableIRQ(EINT1_IRQn);		// disable KEY1 interrupt
	NVIC_DisableIRQ(EINT0_IRQn);
	NVIC_DisableIRQ(EINT2_IRQn);
	
	ChangeMode((mode+1)%NUM_MODE);	// Change turn mode

	// Clear pending interrupt 
	LPC_SC->EXTINT &= (1 << 0);
	LPC_SC->EXTINT &= (1 << 1);
	LPC_SC->EXTINT &= (1 << 2);
	// Avoids to execute the ISR of interrupt triggered while disabled (pending bit could still be set)
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_ClearPendingIRQ(EINT1_IRQn);
	NVIC_ClearPendingIRQ(EINT2_IRQn);
	// Enables again interrupt
	NVIC_EnableIRQ(EINT1_IRQn);  	// Enable mode switch (KEY1)
	NVIC_EnableIRQ(EINT0_IRQn);		// Enable match reset
	if(mode == WALL){
		NVIC_EnableIRQ(EINT2_IRQn);		// Enabled ONLY in WALL mode
	}
}

void EINT2_IRQHandler (void)	  	// KEY2 rotates wall in wall mode
{
	// Disable interrupts
	NVIC_DisableIRQ(EINT1_IRQn);
	NVIC_DisableIRQ(EINT0_IRQn);
	NVIC_DisableIRQ(EINT2_IRQn);
  
	RotateWall();
	
	// Clear pending interrupt 
	LPC_SC->EXTINT &= (1 << 0);
	LPC_SC->EXTINT &= (1 << 1);
	LPC_SC->EXTINT &= (1 << 2);
	// Avoids to execute the ISR of interrupt triggered while disabled (pending bit could still be set)
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_ClearPendingIRQ(EINT1_IRQn);
	NVIC_ClearPendingIRQ(EINT2_IRQn);
	// Enables again interrupt
	NVIC_EnableIRQ(EINT1_IRQn);  	// Enable mode switch (KEY1)
	NVIC_EnableIRQ(EINT0_IRQn);		// Enable match reset
	NVIC_EnableIRQ(EINT2_IRQn);		// Enabled since we are in WALL mode
}


