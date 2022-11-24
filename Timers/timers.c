/*
 * timers.c
 *
 *  Created on: 2 lis 2020
 *      Author: Piotr
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include "timers.h"

		volatile 	uint8_t		Timer2,Timer3,Timer4,Timer5;
		volatile	uint8_t 	n;

 void timer0_init() {

	// Timer0 – konfigurajca silnika timerów programowych
	TCCR0A  |= (1<<WGM01);   // tryb pracy CTC
	TCCR0B  |= (1<<CS02)|(1<<CS00); // preskaler = 1024
	OCR0A  = 179; //Timer0_init(10,-3);   // przerwanie porównania co 10ms (100Hz)    8Mhz - 78           18,432Mhz - 179
	TIMSK0  = (1<<OCIE0A); // Odblokowanie przerwania CompareMatch
}



// przerwanie Timer0 Compare
ISR(TIMER0_COMPA_vect) {	// 100Hz Timer1

//	 n = Timer1;
//	 if (n) Timer1 = --n;
	 n = Timer2;				// Display
	 if (n) Timer2 = --n;
	 n = Timer3;				// PID Regulator
	 if (n) Timer3 = --n;
	 n = Timer4;				// Read Temp
	 if (n) Timer4 = --n;
	 n = Timer5;				// PWM Heating
	 if (n) Timer5 = --n;


}

