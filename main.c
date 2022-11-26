/*
 * main.c
 *
 *  Created on: 23 lis 2022
 *      Author: Piotr
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "timers/timers.h"
#include "ifRS485/ifRS485.h"
#include "ds18b20/ds18b20.h"
#include "ds18b20/pindef.h"
#include "ds18b20/onewire.h"

void	lcd_putULInt_goto( uint32_t, uint8_t);
void	lcd_putInt_goto( int16_t, uint8_t);

#ifndef F_CPU								// if F_CPU was not defined in Project -> Properties
#define F_CPU 18432000UL
#endif

#define BAUD 115200

#define CNTDS 45							// maximum number of sensor

#define PREV_READ 0								// State reading sensor
#define NEW_READ 1							//

#define MYUBRR (F_CPU/16UL/BAUD)-1			// parameter transfered to the procedure initiating RS485 transmision
uint16_t ubrr = MYUBRR;

//		ifRS485
//Konfiguracja UART oraz RS485
#define DDR_Dir		&DDRD
#define PORT_Dir	&PORTD
#define PIN_TX_Dir	(1<<PD2)
#define PIN_RX_Dir	(1<<PD5)
#define BUFOR0_SIZE	64

//TifRS485	if0pcBus;	// interfejsy magistrali RS485
TifRS485 if0pcBus ={ 0, 0, MYUBRR, (uint8_t *)DDR_Dir, (uint8_t *)PORT_Dir, PIN_TX_Dir, PIN_RX_Dir, 0, BUFOR0_SIZE };


//		DS18b20
//const static gpin_t sensorPin = { &PORTD, &PIND, &DDRD, PD5 };
const static gpin_t sensorPin = { &PORTC, &PINC, &DDRC, PC1 };

TAddrSlv	AdresSlv[CNTDS];
uint8_t		StateDS[CNTDS];
float		Temp[CNTDS];

// Timery programowe
extern volatile  uint8_t Timer2,Timer3,Timer4,Timer5;	// znacznik aktywacji przerwania

int main(void) {

	// Inicjalizacja timerów
	timer0_init();				// Podstawa timerów programowych

	sei();

//	TAddrSlv	AdresSlv[CNTDS];
//	uint8_t		StateDS[CNTDS];
//	float		Temp[CNTDS];

	// Prepare a new device search
    onewire_search_state serialDS18b20;
	onewire_search_init(&serialDS18b20);				// Prepare new search

/*	// Configure interface USART						// Temporary writing config in one line
	if0pcBus.numUSART = 0;
	if0pcBus.ubrr = MYUBRR;
	if0pcBus.rsDDR = (uint8_t *)DDR_Dir;
	if0pcBus.rsPort = (uint8_t *)PORT_Dir;
	if0pcBus.rsPinTX_DIR = PIN_TX_Dir;
	if0pcBus.rsPinRX_DIR = PIN_RX_Dir;
	if0pcBus.buf_size = BUFOR0_SIZE;
*/

	// Initialize interface USART
	ifRS485_init ( &if0pcBus );
	char *ReadData = (char *)malloc(16 * sizeof(char));

	for(uint8_t i=0;i<CNTDS;i++) { 						// Filling table of zeros
		StateDS[i]=0;
		for(uint8_t j=0; j<8; j++)
			AdresSlv[i].adres[j]=0;
	}

	uint16_t read;
	uint8_t x1=0,x2=1;
	while(1) {
		ifRS485_send( &if0pcBus, "Series: " );
		lcd_putULInt_goto((int32_t)x2++,10);
		ifRS485_send( &if0pcBus, "\n\n" );
//		for (uint8_t i=0; i<CNTDS; i++)
//			if( StateDS[i] == 3) StateDS[i]=1;
		x1=0;
		onewire_search_init(&serialDS18b20);				// Prepare new search
		while(onewire_search(&sensorPin, &serialDS18b20)) {
	//		ifRS485_send( &if0pcBus, "\n" );
			lcd_putULInt_goto((int32_t)x1++,10);
	//		ifRS485_send( &if0pcBus, " - onewire_search\n" );

			uint8_t match=0;								// 0 - New sensor
			for(uint8_t i=0; i<CNTDS; i++)	{				// Searching in the table of the read sensor
	//			lcd_putULInt_goto((int32_t)i,10);
	//			ifRS485_send( &if0pcBus, "for(i)    " );
				if (AdresSlv[i].adres[0]==0) {	// End of entries
	//				ifRS485_send( &if0pcBus, "AdresSlv=0 " );
					if ( !match ) {							// Write a new sensor to the table
						for (uint8_t j=0; j<8; j++) AdresSlv[i].adres[j]=serialDS18b20.address[j];
	//					ifRS485_send( &if0pcBus, "SaveAdr " );
						StateDS[i] = 1;
					}
					break;
				} else {
					uint8_t diff=0;
					for (uint8_t j=0; j<8; j++) {
						if (AdresSlv[i].adres[j] != serialDS18b20.address[j]) {
							diff=1;
							break;
						}
					}

					if (!diff) {
						StateDS[i] = 3;
						match=1;
						break;
					}
				}
				_delay_ms(20);
			}  // for (i)
		} //while

		ifRS485_send( &if0pcBus, "\n\n* Convert *\n\n" );
		if (onewire_reset(&sensorPin)) {
			ds18b20_convert(&sensorPin);
			_delay_ms(1000);
		}


		for (uint8_t i=0; i<CNTDS; i++) {
			if( StateDS[i] == 3) {
				read = ((float)(ds18b20_read_slave(&sensorPin, AdresSlv[i].adres)));
				if (Temp[i]==0)
					Temp[i]=read;
				else {
					uint16_t tmp = Temp[i]+read;
					Temp[i]=tmp/2;
				}

				lcd_putULInt_goto((int32_t)i,10);
				for (uint8_t y=0; y<8; y++ )
					lcd_putULInt_goto((int32_t)AdresSlv[i].adres[y], 16);
				ifRS485_send( &if0pcBus, " " );
				lcd_putULInt_goto((int32_t)(Temp[i]*10/16),10);
				lcd_putULInt_goto((int32_t)(read*10/16),10);
				lcd_putULInt_goto((int32_t)Temp[i],10);
				lcd_putULInt_goto((int32_t)read,10);
				ifRS485_send( &if0pcBus, "\n" );
			}
		}

		for (uint8_t i=0; i<CNTDS; i++) {
			if (StateDS[i] == 1 && x2 != 2 ) {
				for (uint8_t y=0; y<8; y++ ) {
					lcd_putULInt_goto((int32_t)AdresSlv[i].adres[y], 16);
				//	AdresSlv[i].adres[y]=0;
				}
				ifRS485_send( &if0pcBus, "->" );
				lcd_putULInt_goto((int32_t)Temp[i]*10/16,10);
				lcd_putULInt_goto((int32_t)Temp[i],10);
				ifRS485_send( &if0pcBus, "\n" );
				StateDS[i]=0;
				Temp[i]=0;
				_delay_ms(20);

				ifRS485_send( &if0pcBus, "Next cycle - press <n>\n" );

				uint8_t cnt = ifRS485_read(&if0pcBus, ReadData, 16);
				while ( ReadData[0] != 110 ) {
					uint8_t cnt = ifRS485_read(&if0pcBus, ReadData, 16);
				}
			} // if (StateDS)
		} // for (i)


		ifRS485_send( &if0pcBus, "\n Possible remove sensor - 2 sec.\n\n" );
		_delay_ms(5000);
	}

}

//void FstateDS( uint8_t* state, uint*) {
//	if ( *state == 0 )
//}

void lcd_putULInt_goto( uint32_t liczba, uint8_t sys) {
	char *buf="                    ";
	buf=ultoa(liczba,buf,sys);
	ifRS485_send( &if0pcBus, buf );
	ifRS485_send( &if0pcBus, ":" );
}

void lcd_putInt_goto( int16_t liczba, uint8_t sys) {
	char *buf="                    ";
	if (liczba<0) {
		ifRS485_send( &if0pcBus, "-" );
		liczba=abs(liczba);
	}
	buf=ultoa(liczba,buf,sys);
	ifRS485_send( &if0pcBus, buf );
}
