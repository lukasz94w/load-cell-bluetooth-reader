#include "adc.h"

uint8_t offset_adc=190;
uint16_t ADCvalues[10]; //tablica na wartoœci odczytanych napiêc z ADC
uint16_t min, max; //zmienne przechowuj¹ce wartoœci minimalne i maksymalne (skrajne wartoœci)
uint16_t sum; //zmienna pomocnicza do przechowywania sumy napiêc z 10 próbek
uint16_t o; //odczytana wartoœæ binarna
uint16_t srednia; //zmienna przechowuj¹ca œrednie napiêcie z 8 próbek (po odrzuceniu dwóch skrajnych)

//mozna zmniejszyc zuzycie energii przez zmniejszenie czestotliwosci probkowania (liczby sampli na sekunde)
//ADCB.CTRLB | = ADC_CURRENTLIMITS_HIGH_gc; // 0x60

uint16_t ReadADC() //ADCMode - 1 dla single, 0 dla pomiaru roznicowego
//wybieramy tryb signed w tym trybie wejscie ujemne (-) ADC zostaje dolaczone do GND a do (+) dajemy ktores z napiec ADC0, ADC1 itd
{
	sum=0; //zerowanie na pocz¹tku sum

	for(uint8_t i=0;i<10;i++) //pêtla dla 10 próbek
	{
		SET_ADC(); //ustawienie rejestrów przetwornika
		//CH0RES WAZNE!!! mozna ustawic tez np CH1RES - rezultat z jakiegos kanalu 1
		o=(ADCA.CH0RES-offset_adc); //odjêcie offsetu ADC od wartoœci zmierzonej
		
		if (o>4095)	o=0;  //sprawdzenie czy po odjêciu offsetu nie "przekrêci³o" licznika, max. dopuszczalna wartoœæ to 4095
		ADCvalues[i] = o; //wczytujemy wartoœæ z ADC

		if(i == 0) // jeœli pierwszy pomiar
		{
			min=ADCvalues[i]; //daj go jako max i min na pocz¹tek
			max=ADCvalues[i];
		}

		sum+=ADCvalues[i]; //sumuj napiêcia z ADC
		if(ADCvalues[i]>max) max = ADCvalues[i]; //jesli > max to podmieñ
		if(ADCvalues[i]<min) min = ADCvalues[i]; //jesli < min podmieñ
	}
	srednia=(sum-(min+max))/8; //mamy sumê 10 próbek, odejmujemy od niej skrajne wartoœci min i max, zostaje nam suma z 8 próbek, liczymy z nich œredni¹
	return srednia;
	//sposob na obliczenie napiecia: 1 = 4095
	//1x=4.095, 4.095 zmienia sie
	//x=ADC/1 gdzie ADC wartosc odczytana (nasze o)
	//DZIELNIK 5.5:1!!!!!
}

uint8_t ReadSignatureByte(uint16_t Address)
{
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	uint8_t Result;
	__asm__ ("lpm %0, Z\n" : "=r" (Result) : "z" (Address));
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;
	return Result;
}

void SET_ADC()
{
		//czytanie z przetwornika ADC
		
		if ((ADCA.CTRLA & ADC_ENABLE_bm) == 0)
		{
			ADCA.CTRLA = ADC_ENABLE_bm ; //zalaczenie ADC
			//ADCA.CTRLB = ADC_CONMODE_bm; //tryb ze znakiem. domyslnie jest wlaczony jest bez znaku
			//ADCA.REFCTRL = 0; // wybranie wenwetrznego napiecia  odniesienia 1V
			ADCA.REFCTRL =  ADC_REFSEL_INT1V_gc; //wybieramy napiecie 1V (30 GRUDNIA)
			//ADCA.REFCTRL =  ADC_REFSEL_INTVCC2_gc; //Vcc/2
			
			ADCA.EVCTRL = 0 ; //wylaczenie systemu zdarzen
			//ADCA.PRESCALER = ADC_PRESCALER_DIV32_gc ; //otrzymujemy 2MHz/32=62.5kHz (za malo powinno wynosic 100kHz-1.4MHz)
			ADCA.PRESCALER = ADC_PRESCALER_DIV8_gc ; //otrzymujemy 2MHz/8=250kHz
			//odczytujemy dane kalibracyjne z wewnetrznej pamieci EEPROM (rekompensuja niedopasowanie
			//poprawiaja liniowosc itp)
			ADCA.CALL = ReadSignatureByte(0x20) ; //wpisywanie do rejestu CALL (low)
			ADCA.CALH = ReadSignatureByte(0x21) ; //wczytanie do rejestru CALH (high)
			_delay_us(400); // czekaj co najmniej 25 taktów (25 clocks)
		}
		ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc ; // tryb single (nie roznicowy), wzmocnienie wtedy (gain) zawsze rowne 1
		ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc; //ustawiamy kanal 0
		ADCA.CH0.INTCTRL = 0; //wylaczamy przerwania

		for(uint8_t Waste = 0; Waste<2; Waste++)
		{
			ADCA.CH0.CTRL |= ADC_CH_START_bm; //start konwersji
			while (ADCA.INTFLAGS==0); //czekaj na zakonczenie
			ADCA.INTFLAGS = ADCA.INTFLAGS;
		}	
}