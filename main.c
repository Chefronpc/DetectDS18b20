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

#define CNTDS 32							// maximum number of sensor

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
#define BUFOR0_SIZE	27

//TifRS485	if0pcBus;	// interfejsy magistrali RS485
TifRS485 if0pcBus ={ 0, 0, MYUBRR, (uint8_t *)DDR_Dir, (uint8_t *)PORT_Dir, PIN_TX_Dir, PIN_RX_Dir, 0, BUFOR0_SIZE };


//		DS18b20
//const static gpin_t sensorPin = { &PORTD, &PIND, &DDRD, PD5 };
const static gpin_t sensorPin = { &PORTC, &PINC, &DDRC, PC1 };

/*TAddrSlv	AdresSlv[CNTDS];
uint8_t		StateDS[CNTDS];
uint8_t		flow[CNTDS];
float		Temp[CNTDS];
*/
// Timery programowe
extern volatile  uint8_t Timer2,Timer3,Timer4,Timer5;	// znacznik aktywacji przerwania

int main(void) {

	// Inicjalizacja timerów
	timer0_init();				// Podstawa timerów programowych

	sei();

	static TAddrSlv	AdresSlv[CNTDS];
	static uint8_t		StateDS[CNTDS];
	static uint8_t		flow[CNTDS];
	static uint16_t		Temp[CNTDS];

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
		flow[i]=0;
		for(uint8_t j=0; j<8; j++)
			AdresSlv[i].adres[j]=0;
	}

	static uint16_t read;
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
						if (StateDS[i] == 0 ) { StateDS[i] = 1; flow[i] = 1; }
					}
					break;
				} else {
					uint8_t diff=0;
		//			ifRS485_send( &if0pcBus, "\n" );
					for (uint8_t j=0; j<8; j++) {

			//			_delay_ms(20);
						if (AdresSlv[i].adres[j] != serialDS18b20.address[j]) {
							diff=1;
							break;
						}
					}
			//		lcd_putULInt_goto((int32_t)i,10);
			//		lcd_putULInt_goto((int32_t)diff,10);
			//		ifRS485_send( &if0pcBus, "\n" );
			//		_delay_ms(1);
					if (!diff) {
						ifRS485_send( &if0pcBus, "Match-> " );
						lcd_putULInt_goto((int32_t)i,10);
						lcd_putULInt_goto((int32_t)StateDS[i],10);
						lcd_putULInt_goto((int32_t)flow[i],10);
						_delay_ms(2);

						switch (flow[i]) {
						case 1:
							StateDS[i] = 2; flow[i] = 12;
							break;

						case 12:
							StateDS[i] = 4; flow[i] = 24;
							break;

						case 13:
							StateDS[i] = 2; flow[i] = 32;
							break;

						case 23:
							StateDS[i] = 2; flow[i] = 32;
							break;

						case 24:
							StateDS[i] = 4; flow[i] = 24;
							break;

						case 25:
							StateDS[i] = 2; flow[i] = 52;
							break;

						case 52:
							StateDS[i] = 4; flow[i] = 24;
							break;

						case 55:
							StateDS[i] = 2; flow[i] = 52;
							break;

						case 32:
							StateDS[i] = 4; flow[i] = 24;
							break;

						case 33:
							StateDS[i] = 2; flow[i] = 32;
							break;

						default:
							break;

						}
						lcd_putULInt_goto((int32_t)StateDS[i],10);
						lcd_putULInt_goto((int32_t)flow[i],10);
						ifRS485_send( &if0pcBus, "\n" );
						_delay_ms(2);

						match=1;
						break;
					}
				}
				_delay_ms(2);
			}  // for (i)
		} //while search
		_delay_ms(20);
		ifRS485_send( &if0pcBus, "\n\n Switch states\n" );

		for (uint8_t i=0; i<CNTDS; i++) {

			lcd_putULInt_goto((int32_t)i,10);
			lcd_putULInt_goto((int32_t)StateDS[i],10);
			lcd_putULInt_goto((int32_t)flow[i],10);

			switch (flow[i]) {

			case 12:
				ifRS485_send( &if0pcBus, "Add sensor! press [y]" );
				_delay_ms(50);
				uint8_t cnt = ifRS485_read(&if0pcBus, ReadData, 16);
				while ( ReadData[0] != 121 ) {
					uint8_t cnt = ifRS485_read(&if0pcBus, ReadData, 16);
				}
				break;

			case 24:
				if (StateDS[i] == 2) { StateDS[i] = 3; flow[i] = 23; }
				if (StateDS[i] == 4)   StateDS[i] = 2;
				break;

			case 23:
				StateDS[i] = 3; flow[i] = 33;
				break;

			case 25:
				StateDS[i] = 5; flow[i] = 55;
				break;

			case 32:				//	case only information
				ifRS485_send( &if0pcBus, "Add sensor! press [y]" );
				_delay_ms(50);
				cnt = ifRS485_read(&if0pcBus, ReadData, 16);
				while ( ReadData[0] != 121 ) {
					uint8_t cnt = ifRS485_read(&if0pcBus, ReadData, 16);
				}
				break;

			default:
				break;
			}

			lcd_putULInt_goto((int32_t)StateDS[i],10);
			lcd_putULInt_goto((int32_t)flow[i],10);
			ifRS485_send( &if0pcBus, "\n" );
			_delay_ms(2);
		}

		ifRS485_send( &if0pcBus, "\n\n* Convert *\n\n" );
		if (onewire_reset(&sensorPin)) {
			ds18b20_convert(&sensorPin);
			_delay_ms(1000);
		}

		ifRS485_send( &if0pcBus, "\n\n Print read sensors\n" );
		_delay_ms(50);
		for (uint8_t i=0; i<CNTDS; i++) {
			lcd_putULInt_goto((int32_t)i,10);
			if( StateDS[i] == 2 && flow[i] == 24 ) {
				read = ((ds18b20_read_slave(&sensorPin, AdresSlv[i].adres)));
				if (Temp[i]==0)
					Temp[i]=read;
				if ( ( read > Temp[i]*2) || (read < Temp[i]/2) ) { flow[i] = 25; StateDS[i] = 5; }
				else {
					uint16_t tmp = Temp[i]+read;
					Temp[i]=tmp/2;
				}

				for (uint8_t y=0; y<8; y++ )
					lcd_putULInt_goto((int32_t)AdresSlv[i].adres[y], 16);
				ifRS485_send( &if0pcBus, " " );
				_delay_ms(5);
				lcd_putULInt_goto((int32_t)(Temp[i]*10/16),10);
				lcd_putULInt_goto((int32_t)(read*10/16),10);
				_delay_ms(5);
				lcd_putULInt_goto((int32_t)Temp[i],10);
				lcd_putULInt_goto((int32_t)read,10);
				ifRS485_send( &if0pcBus, "\n" );
			} else {
				if( StateDS[i] == 5 ) {
					ifRS485_send( &if0pcBus, " Err read Temp\n" );
					_delay_ms(50);
				} else {
					ifRS485_send( &if0pcBus, " offline sensor\n" );
					_delay_ms(50);
				}
			}
		}

		ifRS485_send( &if0pcBus, "\n\n Raport loss sensor\n" );
		_delay_ms(50);
		for (uint8_t i=0; i<CNTDS; i++) {
			if (StateDS[i] == 3 && flow[i] == 23) {
			//if (StateDS[i] == 3 && (flow[i] == 13 || flow[i] == 23)) {
				lcd_putULInt_goto((int32_t)i,10);
				for (uint8_t y=0; y<8; y++ ) {
					lcd_putULInt_goto((int32_t)AdresSlv[i].adres[y], 16);
				//	AdresSlv[i].adres[y]=0;
				}
				ifRS485_send( &if0pcBus, "->" );
				lcd_putULInt_goto((int32_t)Temp[i]*10/16,10);
				lcd_putULInt_goto((int32_t)Temp[i],10);
				ifRS485_send( &if0pcBus, "\n" );
				Temp[i]=0;
				_delay_ms(20);

				ifRS485_send( &if0pcBus, "Next cycle press <y>\n" );
				_delay_ms(50);
				uint8_t cnt = ifRS485_read(&if0pcBus, ReadData, 16);
				while ( ReadData[0] != 121 ) {
					uint8_t cnt = ifRS485_read(&if0pcBus, ReadData, 16);
				}
			} // if (StateDS)
		} // for (i)

		_delay_ms(100);
		ifRS485_send( &if0pcBus, "\nPossible remove 5sec\n" );
		_delay_ms(5000);
	} // while (1)

} // main

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
