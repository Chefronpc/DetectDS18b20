/*
 * ifbusrs485.h
 *
 *  Created on: 17 sty 2021
 *      Author: Piotr
 */

#ifndef RS485_IFBUSRS485_H_
#define RS485_IFBUSRS485_H_

#define OK						0
#define WRG_EMPTY_SEND_DATA		1
#define ERR_OVERFLOW_SEND_BUF	2

#define CLR						0
#define SET						1

#define OFF						0
#define ON						1

#define EMPTY					0
#define NOT_EMPTY				1

#define USART0_FRAME_ERROR		UCSR0A & (1<<FE0)
#define USART0_DATA_OVERRUN		UCSR0A & (1<<DOR0)		// Przepe³nienie bufora odbiorczego USART
#define USART0_PARITY_ERROR		UCSR0A & (1<<UPE0)		// B³¹d parzystoœci
#define USART0_DATA				UDR0					// Rejestr I/O

#pragma pack(2)
typedef struct {
	char 		*rx_buf;		// przestrzeñ pamieci dla bufora odbiorczego
	uint8_t  	rx_size;		// wielkoœc bufora
	uint8_t  	rx_read;		// pozycja do odczytu z bufora
	uint8_t  	rx_write;		// pozycja do zapisu do bufora
	uint8_t		rx_write_bck;	// backup pozycji pozycji zapisu w trakcie odbioru pakietu. W przypadku uszkodzonego pakietu jest on odrzucany
								// a pozycja zapisu powraca do wartoœci z backupu
	uint8_t		rx_valid_byte;	// Flaga poprawnego bajtu wejœciowego
	uint8_t  	rx_overflow;	// Flaga ustawiana po przepe³nieniu bufora odbiorczego
	uint8_t		rx_complete;	// Flaga odbioru pe³nego pakietu

	char 		*tx_buf;		//
	uint8_t  	tx_size;		//
	uint8_t  	tx_read;		//
	uint8_t  	tx_write;		//
	uint8_t		tx_write_bck;	// backup pozycji pozycji zapisu w trakcie odbioru pakietu. W przypadku uszkodzonego pakietu jest on odrzucany
								// a pozycja zapisu powraca do wartoœci z backupu
	uint8_t  	tx_overflow;	// Flaga ustawiana po przepe³nieniu bufora nadawczego (Konieczna obs³uga poprzez ponowienie komunikatu)
	uint8_t		tx_complete;	// Flaga zakoñczenia transmisji pe³nego pakietu

} TbufRS485;
#pragma pack()

#pragma pack(2)
typedef struct {
	uint8_t		numUSART;		// numer portu USART
	uint8_t		pbTS_copy;		// Przekazany numer stacji PCBus do warstwy MAC w celu odrzucania pakietów nieadresowanych do stacji
								// ju¿ na etapie przerwañ sprzêtowych i buforu wejœciowego.
	uint16_t	ubrr;			// boud rate
	uint8_t		*rsDDR;			// adres rejestru DDRx
	uint8_t		*rsPort;		// adres rejestru PORTx
	uint8_t		rsPinTX_DIR;		// pin PINx dla aktywacji linii TX
	uint8_t		rsPinRX_DIR;		// pin PINx dla aktywacji linii RX
	uint8_t		PinRX_monitor;	// Pods³uch nadawania uzale¿niony od maszyny stanu warstwy LLC
	uint8_t		buf_size;		// wielkoœc bufora
	uint8_t		state;			// maszyna stanu - detekcja ramki danych
	TbufRS485	bufRS485;		// Definicja kolejeki w danym interfejsie


} TifRS485;
#pragma pack()

void 	ifRS485_init		( TifRS485 *);
uint8_t ifRS485_sizefree	( TifRS485 *);
uint8_t ifRS485_isEmpty		( TifRS485 *);
uint8_t ifRS485_send		( TifRS485 *, char *);
uint8_t ifRS485_read		( TifRS485 *, char *, uint8_t );
/*
void	ifRS485_init		( TifRS485 *);
uint8_t ifRS485_sizefree	( );
uint8_t ifRS485_isfree		( char *);
uint8_t ifRS485_send		( char *);
uint8_t ifRS485_read		( char *, uint8_t );
*/

#endif /* RS485_IFBUSRS485_H_ */
