#ifndef MAIN_H_
#define MAIN_H_

#define F_CPU 2000000UL //zdefiniowany jest we wlasciwosciach projektu wiec teoretycznie nie jest potrzebna kolejna definicja

#include "adc.h"
#include "bt.h"
#include "rtc.h"
#include "twi_master_driver.h"
#include "usart.h"
#include "ADS1220.h"
#include "spi.h"

#include <avr/eeprom.h> //EEPROM
#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h> //do delaya
#include <avr/interrupt.h> //do przerwan

void BT_PC(void);
void init(void);
void init_TCC0(void);
void init_TCC1(void);
void ads_offset_calibration (void);
void ads_init(void);
float ads_measure(void);
float sd24conv(uint32_t, uint8_t, uint32_t, uint32_t, float, float, uint32_t, uint32_t, float, float);
void battery_watchdog(void);
void RTC_SetTime(void);
void pomiar(void);
void funkcja_konfiguracyjna(void);
void funkcja_ramka_danych(void);
//przetwornik ADC
#define _ADCIN PIN0_bm
//przycisk PWR_ON
#define PWR_ON PIN6_bm
//karta sd
#define _SD_ON PIN0_bm
//dioda STATUS
#define STATUS_LED PIN2_bm
//RTC
#define _SDA PIN0_bm
#define _SCL PIN1_bm
//bluetooth UART
#define _TXD0 PIN3_bm
#define _RXD0 PIN2_bm
//USB
#define _TXF0 PIN3_bm
#define _RXF0 PIN2_bm
//bluetooth
#define _BT_V PIN2_bm
#define _BT_RESET PIN3_bm
//makra bluetooth
#define BT_OFF PORTE.OUTSET = _BT_V  //wylaczenie zasilania modu³u BT
#define BT_ON PORTE.OUTCLR = _BT_V  //wlaczenie zasilania modu³u BT
#define BT_RESET_ON PORTB.OUTSET = _BT_RESET  //ustawienie reset
#define BT_RESET_OFF PORTB.OUTCLR = _BT_RESET  //ustawienie reset
//ADS1220
#define _DRDY PIN3_bm
#define _ADS_CS PIN4_bm
#define ADS_MOSI PIN5_bm
#define ADS_MISO PIN6_bm
#define ADS_SCK PIN7_bm
//klawisze funkcyjne
#define _KEY_2 PIN0_bm
#define _KEY_1 PIN1_bm

#endif /* MAIN_H_ */