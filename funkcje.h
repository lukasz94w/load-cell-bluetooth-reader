#ifndef FUNKCJE_H_
#define FUNKCJE_H_

#include "main.h" //stad beda brane makrodefinicje

void my_delay_ms(uint16_t);
void blink (uint8_t, uint16_t, uint16_t);
void power_down(void);
void BT_PC_init(void);
void PC_BT_init(void);
void DEVICE_OFF(void);

#endif /* FUNKCJE_H_ */