#include "usart.h"

//funkcja do wyliczania parametrow UART'a
bool usart_set_baudrate	(USART_t * usart, uint32_t baud, uint32_t cpu_hz)
{
    int8_t exp;
    uint32_t div;
    uint32_t limit;
    uint32_t ratio;
    uint32_t min_rate;
    uint32_t max_rate;

    /*
     * Check if the hardware supports the given baud rate
     */
    /* 8 = (2^0) * 8 * (2^0) = (2^BSCALE_MIN) * 8 * (BSEL_MIN) */
    max_rate = cpu_hz / 8;
    /* 4194304 = (2^7) * 8 * (2^12) = (2^BSCALE_MAX) * 8 * (BSEL_MAX+1) */
    min_rate = cpu_hz / 4194304;

    if (!((usart)->CTRLB & USART_CLK2X_bm)) {
        max_rate /= 2;
        min_rate /= 2;
    }

    if ((baud > max_rate) || (baud < min_rate)) {
        return false;
    }

    /* Check if double speed is enabled. */
    if (!((usart)->CTRLB & USART_CLK2X_bm)) {
        baud *= 2;
    }

    /* Find the lowest possible exponent. */
    limit = 0xfffU >> 4;
    ratio = cpu_hz / baud;

    for (exp = -7; exp < 7; exp++) {
        if (ratio < limit) {
            break;
        }

        limit <<= 1;

        if (exp < -3) {
            limit |= 1;
        }
    }

    /*
     * Depending on the value of exp, scale either the input frequency or
     * the target baud rate. By always scaling upwards, we never introduce
     * any additional inaccuracy.
     *
     * We are including the final divide-by-8 (aka. right-shift-by-3) in
     * this operation as it ensures that we never exceeed 2**32 at any
     * point.
     *
     * The formula for calculating BSEL is slightly different when exp is
     * negative than it is when exp is positive.
     */
    if (exp < 0) {
        /* We are supposed to subtract 1, then apply BSCALE. We want to
         * apply BSCALE first, so we need to turn everything inside the
         * parenthesis into a single fractional expression.
         */
        cpu_hz -= 8 * baud;

        /* If we end up with a left-shift after taking the final
         * divide-by-8 into account, do the shift before the divide.
         * Otherwise, left-shift the denominator instead (effectively
         * resulting in an overall right shift.)
         */
        if (exp <= -3) {
            div = ((cpu_hz << (-exp - 3)) + baud / 2) / baud;
        } else {
            baud <<= exp + 3;
            div = (cpu_hz + baud / 2) / baud;
        }
    } else {
        /* We will always do a right shift in this case, but we need to
         * shift three extra positions because of the divide-by-8.
         */
        baud <<= exp + 3;
        div = (cpu_hz + baud / 2) / baud - 1;
    }

    (usart)->BAUDCTRLB = (uint8_t)(((div >> 8) & 0X0F) | (exp << 4));
    (usart)->BAUDCTRLA = (uint8_t)div;

    return true;
}

void setUpSerial()
{
	//port D	
	usart_set_baudrate(&USARTD0, 115200, 2000000);
	USARTD0_CTRLA = 0; //wylaczenie przerwan (dla pewnosci)
	USARTD0_CTRLC = USART_CHSIZE_8BIT_gc; //8 bitowa ramka, bez bitow parzystosci, 1 bit stopu
	USARTD0_CTRLB = USART_TXEN_bm | USART_RXEN_bm; //zalaczenie nadajnika i odbiornika (i zalaczenie "high speed mode"(?))

	//port F
	usart_set_baudrate(&USARTF0, 115200, 2000000);
	USARTF0_CTRLA = 0; //wylaczenie przerwan (dla pewnosci)
	USARTF0_CTRLC = USART_CHSIZE_8BIT_gc; //8 bitowa ramka, bez bitow parzystosci, 1 bit stopu
	USARTF0_CTRLB = USART_TXEN_bm | USART_RXEN_bm; //zalaczenie nadajnika i odbiornika (i zalaczenie "high speed mode"(?))
}

void sendChar(char c)
{
	while( !(USARTD0_STATUS & USART_DREIF_bm) ); //Wait until DATA buffer is empty
	USARTD0_DATA = c;	
}

void sendCharBT(char c)
{
	while( !(USARTF0_STATUS & USART_DREIF_bm) ); //Wait until DATA buffer is empty
	USARTF0_DATA = c;
}

void sendString(char *text)
{
	while(*text)
	{
		sendChar(*text++);
	}
}

void sendStringBT(char *text)
{
	while(*text)
	{
		sendCharBT(*text++);
	}
}

//funkcja realizujaca zapis do strumienia bajt danych
//przyjmuje jako argument dane o typie char, zostana one 
//zapisane do strumienia wskazanego przez FILE
//w razie prawidlowego zapisu zwracana jest wartosc 0,
//w przypadku bledu - wartosc niezerowa
//inaczej:funkcja wysyla znak do bufora USART i zwraca kod pomyslnego zakonczenia operacji (wartosc 0)
int uart_putchar (char c, FILE *stream)
{
	if (c == '\n')
	uart_putchar('\r', stream);
	// Wait for the transmit buffer to be empty
	while (  !(USARTD0_STATUS & USART_DREIF_bm) );
	// Put our character into the transmit buffer
	USARTD0_DATA = c;
	return 0;
}

int uart_putcharBT (char c, FILE *stream)
{
	if (c == '\n')
	uart_putcharBT('\r', stream);
	// Wait for the transmit buffer to be empty
	while (  !(USARTF0_STATUS & USART_DREIF_bm) );
	// Put our character into the transmit buffer
	USARTF0_DATA = c;
	return 0;
}

//funkcja odbierajaca bajt danych
char usart_receiveByte()
{
	while( !(USARTD0_STATUS & USART_RXCIF_bm) ); //Interesting DRIF didn't work.
	return USARTD0_DATA;
}

char usart_receiveByteBT()
{
	while( !(USARTF0_STATUS & USART_RXCIF_bm) ); //Interesting DRIF didn't work.
	return USARTF0_DATA;
}

//funkcja umozliwiajaca odczyt danych ze strumienia, przyjmuje
//jako argument wskaznik do struktury FILE, a zwraca odczytany znak
//lub:
//_FDEV_ERR - jesli wystapil blad
//_FDEV_EDF - jesli w strumieniu nie ma wiecej danych do odczytu
//inaczej:funkcja czeka na nadejscie znaku, po czym zwraca jego kod
int uart_getchar(FILE *stream)
{
	while( !(USARTD0_STATUS & USART_RXCIF_bm) ); //Interesting DRIF didn't work.
	char data = USARTD0_DATA;
	if(data == '\r')
	data = '\n';
	uart_putchar(data, stream);
	return data;
}

int uart_getcharBT(FILE *stream)
{
	while( !(USARTF0_STATUS & USART_RXCIF_bm) ); //Interesting DRIF didn't work.
	char data = USARTF0_DATA;
	if(data == '\r')
	data = '\n';
	uart_putcharBT(data, stream);
	return data;
}

