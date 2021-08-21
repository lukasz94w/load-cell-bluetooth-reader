#include "main.h"

//konfiguracja strumieni (jeden do PC-USB, drugi do BT)
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
FILE uart_strBT = FDEV_SETUP_STREAM(uart_putcharBT, uart_getcharBT, _FDEV_SETUP_RW);

//zmienne do obslugi ADS1220, oraz obliczania napiecia zasilania
float nap_1; //zmienna zadeklarowana globalnie (gdy byla zadeklarowana w int_main() byla lokalna i nie byla widoczna w przerwaniu (np TCC1_OVF_vect)
float nap_2;
float nap_3;
float nap_4;
float nap_5;
float g;
uint32_t _u_24;  //jak w przykladzie powyzej
float bat_lvl;
float napiecie;

//dwa bufory do obslugi komend i wyliczenia wagi
char str [4];
char spr [4];

//obsluga EEPROM
EEMEM float nap_3_EEPROM;
EEMEM float nap_4_EEPROM;
EEMEM float g_EEPROM;
float nap_33;
float nap_44;
float gg;

//flagi do obslugi MENU
uint8_t error;
uint8_t flag;

//zmienne do ustawiania czasu
unsigned char sekunda;
unsigned char minuta;
unsigned char godzina;
unsigned char dzien;
unsigned char miesiac;
unsigned int rok;

//funkcja do komunikacji BT-PC
void BT_PC()
{
	char str [80];
	stdout = stdin = &uart_strBT;
	scanf ("%79s",str);
	printf ("%s ",str);
	stdout = stdin = &uart_str;
	printf ("%s ",str);
	stdout = stdin = &uart_strBT;
}

//inicjalizacja przerwania od przepelnienia TCC0 (nalezy uzyc funkcji gdy korzystam z wektora przerwania ISR(TCC0_OVF_vect)
//nalezy wtedy wylaczyc funkcje PC_BT() i BT_PC()
void init_TCC0()
{
	//konfiguracja przerwania od przepelnienia
	TCC0.INTCTRLA     =    TC_OVFINTLVL_LO_gc; //przepe³nienie ma generowaæ przerwanie LO
	//PMIC.CTRL         =    PMIC_LOLVLEN_bm; //odblokowanie przerwañ o priorytecie LO - robimy to w init() wiec tutaj wykomentowane
	//sei();

	//konfiguracja timera
	TCC0.CTRLB        =    TC_WGMODE_NORMAL_gc; //tryb normalny
	//TCC0.CTRLFSET     =    TC0_DIR_bm; //liczenie w dó³
	TCC0.CTRLA        =    TC_CLKSEL_DIV1024_gc; //ustawienie preskalera i uruchomienie timera
	TCC0.PER = 2000; //wartosc rejestru PER (do ktorej zlicza timer i generuje przerwanie gdy zliczy) PER=2000 co sekunde, PER=20000 co 10 sekund
}

void init_TCC1()
{
	//konfiguracja przerwania od przepelnienia
	TCC1.INTCTRLA     =    TC_OVFINTLVL_LO_gc; //przepe³nienie ma generowaæ przerwanie LO
	//PMIC.CTRL         =    PMIC_LOLVLEN_bm; //odblokowanie przerwañ o priorytecie LO - robimy to w init() wiec tutaj wykomentowane
	//sei();

	//konfiguracja timera
	TCC1.CTRLB        =    TC_WGMODE_NORMAL_gc; //tryb normalny
	//TCC1.CTRLFSET     =    TC0_DIR_bm; //liczenie w dó³
	TCC1.CTRLA        =    TC_CLKSEL_DIV1024_gc; //ustawienie preskalera i uruchomienie timera
	TCC1.PER = 2000; //wartosc rejestru PER (do ktorej zlicza timer i generuje przerwanie gdy zliczy) PER=2000 co sekunde, PER=20000 co 10 sekund
}

//przerwanie od przepelnienia TCC0, cialo funkcji obslugi przerwania
ISR(TCC0_OVF_vect)
{
	asm ("nop");
}

ISR(TCC1_OVF_vect)
{
	asm ("nop");
}

void init ()
{
	//przetwornik ADC
	PORTA.DIRCLR = _ADCIN; //konfiguracja PIN0 jako wejscie (do pomiaru napiecia)
	//obsluga PW_ON
	PORTD.DIRSET = PWR_ON; //ustawienie jako wyjscie
	PORTD.OUTSET = PWR_ON; //podtrzymanie zasilania ukladu po uruchomieniu
	//obsluga karty SD
	PORTD.DIRSET = _SD_ON; //ustawienie jako wyjscie
	PORTD.OUTSET = _SD_ON; //wylaczenie zasilania karty
	//obsluga diody STATUS
	PORTB.DIRSET = STATUS_LED; //ustawienie kierunku wyjsciowego w celu mozliwosci sterowania dioda STATUS
	//I2C
	PORTC.DIRSET = _SDA | _SCL; //ustawienie jako wyjscie
	//UART PC-USB
	PORTD.DIRSET = _TXD0; //kierunek wyjsciowy
	PORTD.OUTSET = _TXD0; //zalecane jest aby domyslnym stanem wyjsciowym pinu TxD byl stan wysoki
	PORTD.OUTCLR = _RXD0;
	//obsluga klawiszy
	PORTE.PIN0CTRL = PORT_OPC_PULLUP_gc; //podciagniecie do zasilania dla KEY_2
	PORTE.PIN1CTRL = PORT_OPC_PULLUP_gc; //podciagniecie do zasilania dla KEY_1
	//Bluetooth:
	PORTB.DIRSET = _BT_RESET; //kierunek wyjsciowy (do sterowania resetem na module bluetooth)
	PORTE.DIRSET = _BT_V; //kierunek wyjsciowy (do sterowania zasilaniem modulu bluetooth)
	//UART Bluetooth
	PORTF.DIRSET = _TXF0; //kierunek wyjsciowy
	PORTF.OUTSET = _TXF0; //zalecane jest aby domyslnym stanem wyjsciowym pinu TxD byl stan wysoki
	PORTF.OUTCLR = _RXF0;
	//inicjalizacja RTC
	rtc_init();
	//inicjalizacja UART
	setUpSerial();
	//twardy reset Bluetooth
	bt_factory_reset();
	//zalaczenie SPI
	SpiInit();
	PMIC.CTRL |= PMIC_LOLVLEN_bm; //zalaczenie i odblokowanie przerwan (miedzy innymi do RTC i do init_TCC0())
	sei();

	//wysylanie z terminala BT na PC (po wybraniu send)
	//BT_PC_init(); //trzeba jeszcze zalaczyc funkcje BT_PC();
	//wysylanie znak po znaku z kompa na terminal - konfiguracja modulu (patrz opis funkcji PC_BT_init)
	//PC_BT_init(); //WAZNE: funkcja dziala jednak moga byc problemy z roznymi terminalami, na jednym dziala na drugim moze nie dzialac!
	//wyswietlania tego co w przerwaniu (np napiecia i czasu) - ogolnie do sprawdzenia dzialania przerwan
	//init_TCC0();
	//init_TCC1();

	//RTC_SetTime(); //funkcja ta musi byc po zalaczeniu przerwan, bez tego nie ma prawa dzialac!!!!
}

void ads_offset_calibration()
{
	Setup_ADS1220(ADS1220_MUX_SHORTED, ADS1220_OP_MODE_NORMAL, ADS1220_CONVERSION_SINGLE_SHOT,
	ADS1220_DATA_RATE_20SPS, ADS1220_GAIN_128, ADS1220_USE_PGA, ADS1220_IDAC1_DISABLED,
	ADS1220_IDAC2_DISABLED, ADS1220_IDAC_CURRENT_OFF, ADS1220_VREF_EXT_REF1_PINS);
}

void ads_init()
{
	Setup_ADS1220(ADS1220_MUX_AIN1_AIN2, ADS1220_OP_MODE_NORMAL, ADS1220_CONVERSION_SINGLE_SHOT,
	ADS1220_DATA_RATE_20SPS, ADS1220_GAIN_128, ADS1220_USE_PGA, ADS1220_IDAC1_DISABLED,
	ADS1220_IDAC2_DISABLED, ADS1220_IDAC_CURRENT_OFF, ADS1220_VREF_EXT_REF1_PINS);
	//unsigned char b[4]={0x3e,0x04,0x98,0x00}; //dane z noty str 57 reczne ustawienie z pominieciem funkcji setup
	//ADS1220_Write_Regs ((&b), 0, 4);
}

//w tej funkcji przekazujemy gain - jest on w sumie niepotrzebny do niczego bo juz nie dzielimy przez niego (wzmocnienie okreslamy TYLKO w funkcji Setup_ADS1220)
float ads_measure()
{
	//unsigned char b[1]={0xe};
	//ADS1220_Write_Regs ((&b), 0, 1);
	unsigned char a[3]; //tablica 3 bajtowa do zapisu 24 bitowego wyniku z przetwornika
	while (PORTE.IN & _DRDY)	{;}	//oczekiwanie na nowe dane (czy _DRDY zmienilo stan na niski) u¿ycie przy continues mode chociaz bez debaga tez dziala na single shoot
	//my_delay_ms(50); //zamiast oczekiwania na zakoñczenie konwersji w trybie single shot
	ADS1220_Get_Conversion_Data(&a);// get the data from the ADS1220
	_u_24=(a[0]<<8|a[1]); //zmienna do przechowywania wyniku z ADS1220
	_u_24=(_u_24<<8)|a[2]; //za³adowanie tablicy 3x8bit do jednego elementu 24bit
	_u_24=(_u_24 & 0x00FFFFFF); //wyzerowanie bitów 31-24
	
	//POPRAWIONO ZAKRESY W TEJ FUNKCJI TERAZ JEST WSZYSTKO DOBRZE)
	float nap_1= sd24conv(_u_24, 1, 0x00800000, 0x00ffffff, -0.0390625, -0.00000000465661287, 0x0, 0x007FFFFF, 0, 0.0390624953433871); //wywolanie funkcji sd24conv (trzeba pamietac ze dzieli nam tylko wynik w woltach, binarnie nie dzielimy!)
	//tu nie powinno byc nap_1 dla czytelnosci kodu ale dziala
	return nap_1;
}

//konwersja danych binarnc z sd24 na napiêcie
//zwroci wartoœæ napiêcia ze znakiem 0V do 5V typu float
//przyjmie uint32 aktualnie odczytana wartoœæ, uint8 wzmocnienie,
//uint32 binarnie poczatek 1 zakresu, uint32 binarnie koniec 1 zakresu, float wartoœæ napiêcia dla poczatku 1 zakresu, float wartoœæ napiêcia dla konca 1 zakresu
//uint32 binarnie poczatek 2 zakresu, uint32 binarnie koniec 2 zakresu, float wartoœæ napiêcia dla poczatku 2 zakresu, float wartoœæ napiêcia dla konca 2 zakresu
float sd24conv (uint32_t actual_bin, uint8_t gain, //gain w sumie niepotrzebny tutaj jest ale niech bedzie (30 GRUDNIA)
				uint32_t start_1range_bin,  uint32_t end_1range_bin,  float start_1range_voltage, float end_1range_voltage,
				uint32_t start_2range_bin,  uint32_t end_2range_bin,  float start_2range_voltage, float end_2range_voltage)
{
	if ((actual_bin >= start_1range_bin) && (actual_bin <= end_1range_bin)) //sprawdzenie czy wynik miesci sie w pierwszym zakresie
	{
		float  result_X=((end_1range_voltage-start_1range_voltage)/(end_1range_bin-start_1range_bin))*(actual_bin-start_1range_bin)+start_1range_voltage; //obliczenia ze wzoru: [(y2-y1)/(x2-x1)]*(x-x1)+y1		
		return result_X;
	}
	
	else
	{
		float  result_Y=((end_2range_voltage-start_2range_voltage)/(end_2range_bin-start_2range_bin))*(actual_bin-start_2range_bin)+start_2range_voltage;
		return result_Y;
	}
}

//WYLICZANIE SPADKU NAPIECIA NA DIODZIE (uwzgledniane przy wyliczaniu napiecia w funkcja_ramka_danych)
//obliczenia dla USB: 5-4.65=0.35!		   
//obliczenia dla baterii: 4.128-3.78=0.348 
//przyjmuje ze spadek wynosi okolo 0.35V!! (USB ma wydajnosc pradowa 0.5A - pasuje)

//w nocie podane jest napiecie baterii (cutoff) 3V albo 2.7V, ustawie 3.2V
void battery_watchdog()
{
	if(napiecie<=3.2)
	{
		power_down();
	}
}

void RTC_SetTime()
{
	g_date_time.second	= sekunda;
	g_date_time.minute	= minuta;
	g_date_time.hour	= godzina;
	g_date_time.day		= dzien;
	g_date_time.month	= miesiac;
	g_date_time.year	= rok;
	g_date_time.weekday	= 1;
	rtc_save_date_time(&g_date_time);
}

void pomiar()
{
	ads_offset_calibration ();
	ADS1220_Start();
	nap_1=ads_measure();
	ads_init();
	ADS1220_Start();
	nap_2=ads_measure();
	nap_5 = nap_2-nap_1; //wyliczamy napiecie

	float wynik = ((gg-0)/(nap_44-nap_33))*(nap_5-nap_33)+0;
	//printf("waga: %.2fg\n", wynik);
	//printf("Waga: %.0fg\n", wynik); //normalne wyswietlanie
	printf("%0.5f,%.12f,", wynik, nap_5); //do ramki danych
}

void funkcja_konfiguracyjna()
{
	if((flag=1)) //KEY_2, dalem dwa nawiasy bo jakis warning wywalalo z jednym
	{	
		//printf("OK\n");
		scanf ("%s", &str);

		if ((str[0]==107)&&(str[1]==97)&&(str[2]==108)&&(str[3]==105)&&(str[4]==98)&&(str[5]==114)&&(str[6]==97)&&(str[7]==99)&&(str[8]==106)&&(str[9]==97)) //kalibracja
		{
			//printf("pozostaw belk\xA9 nieobci\xA5\xBEon\xA5, wy\x98lij \"ok\" zeby potwierdzi\x86:\n");
			printf("pozostaw belke nieobciazona, wyslij \"ok\" zeby potwierdzic:\n");
			scanf ("%s", &str);

			if((str[0]==111)&&(str[1]==107)) //ok
			{
				ads_offset_calibration ();
				ADS1220_Start();
				nap_1=ads_measure();
				ads_init();
				ADS1220_Start();
				nap_2=ads_measure();
				nap_3 = nap_2-nap_1; //napiecie dla stanu zerowego

				eeprom_update_float(&nap_3_EEPROM, nap_3); //zapisuje nap_3 do pamieci
				nap_33=eeprom_read_float(&nap_3_EEPROM); //wczytuje od razu

				//printf("napiecie dla belki nieobci\xA5\xBEonej: %.8fV\n", nap_3);
				printf("napiecie dla belki nieobciazonej: %.8fV\n", nap_3);
				//printf("po\x88\xA2\xBE odwa\xBEnik i podaj jego wag\xA9 w formacie xxx[g]:\n");
				printf("poloz odwaznik i podaj jego wage w formacie xxx[g]:\n");
				scanf ("%s", &spr);
				g=(spr[0]-48)*100+(spr[1]-48)*10+(spr[2]-48); //wyliczam wage podana w formacie xxx[g]

				eeprom_update_float(&g_EEPROM, g); 
				gg=eeprom_read_float(&g_EEPROM);

				//printf("podano wag\xA9: %.1fg\n", g);
				printf("podano wage: %.1fg\n", g);
				ads_offset_calibration ();
				ADS1220_Start();
				nap_1=ads_measure();
				ads_init();
				ADS1220_Start();
				nap_2=ads_measure();
				nap_4 = nap_2-nap_1; //napiecie dla podanej wagi

				eeprom_update_float(&nap_4_EEPROM, nap_4); 
				nap_44=eeprom_read_float(&nap_4_EEPROM);

				printf("napiecie dla %1.fg: %.8fV\n", g, nap_4);
				flag=0;
			}

			else 
			{
				printf("nie potwierdzono polecenia\n");
				flag=0;
			}	
		}

		else if ((str[0]==99)&&(str[1]==122)&&(str[2]==97)&&(str[3]==115)) //czas
		{
			//printf("podaj czas i dat\xA9 w formacie HH:MM:SS:DD:MM:YY\n");
			printf("podaj czas i date w formacie HH:MM:SS:DD:MM:YY\n");
			scanf("%hhu:%hhu:%hhu:%hhu:%hhu:%u", &godzina, &minuta, &sekunda, &dzien, &miesiac, &rok);
			RTC_SetTime();
			flag=0;
		}

// 		else if ((str[0]==112)&&(str[1]==111)&&(str[2]==119)&&(str[3]==101)&&(str[4]==114)&&(str[5]==100)&&(str[6]==111)&&(str[7]==119)&&(str[8]==110)) //powerdown
// 		{
// 			power_down();
// 			flag=0;
// 		}

		else 
		{	
			if (error>0)
			{
				printf("nieznana komenda\n");	
				flag=0;
			}
			error++;
		}
	}	
}

void funkcja_ramka_danych()
{
	while (flag==0)
	{
		bat_lvl=ReadADC(); //odczytane napiecie (z zakresu 0-4095 - trzeba to przeliczyc)
		napiecie = (((1*bat_lvl)/4096)*5.5)+0.35; //wyliczamy napiecie (dodajemy spadek napiecia na diodzie PMEG - 0.35V, bo chcemy znac wartosc napiecia z baterii i przy okazji z USB)
		battery_watchdog();
		
		rtc_read_date_time(); //odczyt zegara z RTC (odczyt zegara na 8MHz trwa 1ms)
		
		pomiar();
		
		PORTB.OUTTGL = STATUS_LED; //miganie diody za kazdym razem gdy wysyla dane
		
		printf("%hhu:%hhu:%hhu,%hhu:%hhu:20%u,%.2f\n", g_date_time.hour, g_date_time.minute, g_date_time.second, g_date_time.day, g_date_time.month, g_date_time.year, napiecie); //HH:MM:SS<>DD:MM:YYYY<>VCC
		my_delay_ms(725); //dobre miejsce na wstawienie 

		if(!(PORTE.IN & _KEY_2))
		{	
// 			my_delay_ms(5000);
// 			if(!(PORTE.IN & _KEY_2))
// 			{
// 				power_down();
// 			}
// 
// 			else
// 			{
				flag=1; //kasuje flage i wychodze z petli
				error=0; //kasuje flage przed wejsciem do funkcji konfiguracyjnej (powiazane z komunikatem "nieznane polecenie")
//			}
		}

		else if(!(PORTE.IN & _KEY_1)) //w petli czekamy az pamiec zapisze dane (unikamy uszkodzenia pamieci w przypadku bez petli while!!!!)
		{
			flag=2;
			while (flag==2)
			{
				ads_offset_calibration ();
				ADS1220_Start();
			 	nap_1=ads_measure();
			 	ads_init();
			 	ADS1220_Start();
			 	nap_2=ads_measure();
			 	nap_3 = nap_2-nap_1; //napiecie dla stanu zerowego
				eeprom_update_float(&nap_3_EEPROM, nap_3); //zapisuje nap_3 do pamieci
				nap_33=eeprom_read_float(&nap_3_EEPROM); //wczytuje od razy z pamieci zapisana wartosc
				flag=0;
			}
 		}

		else
		{
			asm("nop");
		}
	}
}

int main(void)
{
	init();
	//przed uruchomieniem funkcji (z pomiarem napiecia), wczytanie danych z eepromu (bylo w while)
	nap_33=eeprom_read_float(&nap_3_EEPROM);
	nap_44=eeprom_read_float(&nap_4_EEPROM);
	gg=eeprom_read_float(&g_EEPROM);
	//otwieram strumien BT
	PORTD.DIRCLR = _TXD0; //kierunek wejsciowy
	PORTD.OUTCLR= _TXD0; //stan niski
	stdout = stdin = &uart_strBT;
	while (1)
	{
		//BT_PC(); //trzeba jeszcze zalaczyc funkcje BT_PC_init();
		//blink(1,250,250);
		//DEVICE_OFF(); //przerobic zeby dzialalo na PWR_ON?
		funkcja_ramka_danych();
		funkcja_konfiguracyjna();
	}
}
