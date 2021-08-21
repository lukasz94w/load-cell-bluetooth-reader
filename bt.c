#include "bt.h"

void bt_factory_reset()
{
	BT_OFF; //wylaczenie bluetooth  
	BT_ON; 
	my_delay_ms(300);
	BT_RESET_OFF; 
	my_delay_ms(100);
	BT_RESET_ON;
}
