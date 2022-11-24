/*
 * ifbusrs485.c	ver. 1.1
 *
 *  Created on: 17 sty 2021
 *      Author: Piotr
 */

#include <avr/io.h>
#include <avr/interrupt.h>


#include <stdlib.h>
#include <string.h>
#include "ifRS485.h"
#include "../timers/timers.h"

uint16_t	ubrr;

// testowe
//static volatile uint16_t	chk=0;
volatile uint32_t	tim1,tim2,tim2_max,max_tim2 = 0, tim1_en=CLR;
volatile uint16_t	cnt_byte=0;

// Definicje zmiennych dla Interfejsu RS485
static volatile TifRS485	*ifRS;						// lokalny wskaŸnik na interfejs - dostêpnoœc z poziomu ISR oraz funkcji
static volatile uint8_t		znak;						// Tymczasowy bufor odebranego znaku - zapis w ISR, u¿ywany w procedurach maszyny stanu



void ifRS485_init( TifRS485 *tmifRS) {


	ifRS = tmifRS;													// Kopia wskaznika na kolejkê sprzêtow¹ na potrzeby obs³ugi przerwañ

	ifRS->bufRS485.rx_buf = (char *)malloc(ifRS->buf_size * sizeof(char));
	ifRS->bufRS485.rx_size = ifRS->buf_size;
	ifRS->bufRS485.rx_read = 0;
	ifRS->bufRS485.rx_write = 0;
	ifRS->bufRS485.rx_write_bck = 0;
	ifRS->bufRS485.rx_overflow = CLR;
	ifRS->bufRS485.rx_complete = 1;

	ifRS->bufRS485.tx_buf = (char *)malloc(ifRS->buf_size * sizeof(char));
	ifRS->bufRS485.tx_size = ifRS->buf_size;
	ifRS->bufRS485.tx_read = 0;
	ifRS->bufRS485.tx_write = 0;
	ifRS->bufRS485.tx_write_bck = 0;
	ifRS->bufRS485.tx_overflow = CLR;

	ifRS->PinRX_monitor = OFF;										// Domyœlnie transmisja TX bez pods³uchu nadawania, wy³¹czona transmisja RX .

	ifRS->state = 1;			// poczÄ…tek maszyny stanu

	*ifRS->rsDDR |= ifRS->rsPinTX_DIR; 									// Ustawienie pinu TX jako wyjœcie
	*ifRS->rsDDR |= ifRS->rsPinRX_DIR; 									// Ustawienie pinu RX jako wyjœcie
	*ifRS->rsPort &= ~ifRS->rsPinTX_DIR;								// High Z
	*ifRS->rsPort |= ifRS->rsPinRX_DIR;									// High Z

	UBRR0H = (uint8_t)(ifRS->ubrr>>8);								// Ustawianie BitRate
	UBRR0L = (uint8_t) ifRS->ubrr;
//	UCSR0A |= _BV(U2X0);											// Podwójna prêdkœc transmisji
	UCSR0A &= ~_BV(U2X0);											// Pojedyncza prêdkoœc transmisji

//	UCSR0C |= _BV(UPM01);
	UCSR0C &= ~_BV(UPM01);

//	UCSR0C |= _BV(USBS0);
	UCSR0B = _BV(RXCIE0)|_BV(TXCIE0)|_BV(RXEN0)|_BV(TXEN0);			// W³¹czenie przerwañ <obioru danych>, <zakoñczenia transmisji>
																	// W³¹czenie transmisji TX i RX.


	UCSR0C =_BV(UCSZ01)|_BV(UCSZ00);								// 8bitów danych,  1bit stopu


}


uint8_t ifRS485_isEmpty(TifRS485 *tmifRS) {						// Weryfikacja wolnej przestrzeni bufora dla wysy³anych danych
																// Weryfikacja wolnej przestrzeni bufora dla wysy³anych danych
	ifRS = tmifRS;

	if (ifRS->bufRS485.tx_write == ifRS->bufRS485.tx_read) return EMPTY;

	return NOT_EMPTY;
}



uint8_t ifRS485_send(TifRS485 *tmifRS, char *p) {

	ifRS = tmifRS;

	uint8_t next;													// lokalny znaczmik nastêpnej pozycji od pocz¹tkowego lokalizacji bufora
	uint8_t size_free=0;											// Iloœc wolnego miejsca w buforze
	uint8_t diff_wr_rd=0,diff_rd_wr=0;								// Ró¿nice w pozycji znaczników zapisu i odczytu

	if (*p == '\0')													// zwraca b³¹d przy braku danych
		return WRG_EMPTY_SEND_DATA;


	if (ifRS->bufRS485.tx_write > ifRS->bufRS485.tx_read)			// Obliczanie wolnego miejsca w buforze nadawczym
		diff_wr_rd = ifRS->bufRS485.tx_write-ifRS->bufRS485.tx_read;
	else
		if (ifRS->bufRS485.tx_write < ifRS->bufRS485.tx_read)
			diff_rd_wr = ifRS->bufRS485.tx_read-ifRS->bufRS485.tx_write;

	if (diff_rd_wr==0 && diff_wr_rd==0 )
		size_free=ifRS->bufRS485.tx_size-1;

	else {
		if (diff_wr_rd>0) {
			size_free=ifRS->bufRS485.tx_size-diff_wr_rd-1;
		}
		else {
			size_free=diff_rd_wr-1;
		}
	}

	if ((strlen(p)) > size_free) {									// Gdy dane przekraczaj¹ wolne miejsce w buforze - anulowanie transmisji

		return ERR_OVERFLOW_SEND_BUF;
	}

	next = (ifRS->bufRS485.tx_write + 1)%ifRS->bufRS485.tx_size;

	while (next != ifRS->bufRS485.tx_read && *p != '\0') {			// Przepisuje wysy³any ci¹g do cyklicznego bufora nadawczego
		ifRS->bufRS485.tx_buf[ifRS->bufRS485.tx_write] = *p;
		p++;
		ifRS->bufRS485.tx_write = next;
		next = (ifRS->bufRS485.tx_write + 1)%ifRS->bufRS485.tx_size;
	}

	sei();
	if (*p != '\0')	{												// Jeœli zakoñczenie przepisywania zakoñczy³o siê nie znakiem koñca ci¹gu
																	// to oznacza przepe³nienie bufora nadawczego,  zwraca b³¹d "2"
		return ERR_OVERFLOW_SEND_BUF;								// Znacznik pozycji zapisu bufora waraca do pierwotnego stanu
	}

//	*ifRS->rsPort &= ~(ifRS->rsPinRX_DIR);							// TEMP Pods³uch linii przy nadawaniu TESTY
//	*ifRS->rsPort |= (ifRS->rsPinTX_DIR);							// Docelowo nas³uch pozostawiony przy nadawaniu wy³¹cznie TOKENA

	//UCSR0B &= ~_BV(RXEN0);										// Wy³¹czenie nadajnika USART
	*ifRS->rsPort &= ~ifRS->rsPinTX_DIR;									// High Z
	*ifRS->rsPort |= ifRS->rsPinRX_DIR;									// High Z

	UCSR0B |= _BV(UDRIE0);											// w³¹czenie przerwania pustego bufora nadawczego USART
	ifRS->bufRS485.tx_complete = CLR;								//
	return OK;
}

uint8_t ifRS485_read(TifRS485 *tmifRS, char *buf, uint8_t size) {	// Zwraca zadanÄ… liczbÄ™ (size) znakÃ³w,
																	// Zwraca zadanÄ… liczbÄ™ (size) znakÃ³w,
																	// lub mniejszÄ… - do oprÃ³Å¼nienia bufora.
	ifRS = tmifRS;



	uint8_t n = 0;

	while (ifRS->bufRS485.rx_read != ifRS->bufRS485.rx_write && n < size) {
		buf[n++] = ifRS->bufRS485.rx_buf[ifRS->bufRS485.rx_read];
		ifRS->bufRS485.rx_read = (ifRS->bufRS485.rx_read + 1)%ifRS->bufRS485.rx_size;
	}

	sei();
	buf[n] = '\0';

	ifRS->bufRS485.rx_complete = CLR;								// Skasowanie flagi odbioru pakietu

	return n;
}




ISR(USART_RX_vect)													// Odczyt z bufora UART do bufora odbiorczego
{

	uint8_t next;													// lokalny znaczmik nastêpnej pozycji od pocz¹tku bufora

	znak = USART0_DATA;

    next = (ifRS->bufRS485.rx_write + 1)%ifRS->bufRS485.rx_size;
	if (next != ifRS->bufRS485.rx_read) {
		ifRS->bufRS485.rx_buf[ifRS->bufRS485.rx_write] = znak;	// wolne miejsce w buforze do zapisu znaku
		ifRS->bufRS485.rx_write = next;
	} else {
		ifRS->bufRS485.rx_overflow = SET;				// przepe³nienie bufora odbiorczego, utrata danych
	}

}



ISR(USART_TX_vect)
{


	if (!( UCSR0B & _BV(UDRIE0))) {


	//	*ifRS->rsPort &= ~(ifRS->rsPinTX_DIR|ifRS->rsPinRX_DIR);	// Prze³¹czenie w tryb odbioru natychmiast po wys³aniu ostatniego bitu pakietu danych
		UCSR0B |= _BV(RXEN0);								// Za³¹cznie Odbiornika USART
	}

}

ISR(USART_UDRE_vect)
{


	if (ifRS->bufRS485.tx_read == ifRS->bufRS485.tx_write) {
		UCSR0B &= ~_BV(UDRIE0);								// Po opró¿nieniu bufora nadawczego, wy³¹czenie przerwania nadawania
		ifRS->bufRS485.tx_complete = SET;
	} else {

		UDR0 = ifRS->bufRS485.tx_buf[ifRS->bufRS485.tx_read];
		ifRS->bufRS485.tx_read = (ifRS->bufRS485.tx_read + 1)%ifRS->bufRS485.tx_size;
		cnt_byte++;
	}

}


