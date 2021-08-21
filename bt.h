#ifndef BT_H_
#define BT_H_

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "main.h" //potrzebne do makrodefinicji uzytych w funkcji bt_factory_reset()

void bt_factory_reset();

#endif /* BT_H_ */