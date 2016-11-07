/*
 * evt_mask.h
 *
 *  Created on: Apr 12, 2013
 *      Author: g.kruglov
 */

#pragma once

// Event masks
#define EVT_UART_NEW_CMD    EVENT_MASK(1)
#define EVT_BUTTONS         EVENT_MASK(2)
#define EVT_OFF             EVENT_MASK(3)
#define EVT_PILL_CHECK      EVENT_MASK(4)
#define EVT_LED_SEQ_END     EVENT_MASK(5)
#define EVT_VIBRO_END       EVENT_MASK(6)

#define EVT_RXCHECK         EVENT_MASK(8)

// App-specific
#define EVT_RX              EVENT_MASK(10)
#define EVT_CSTATE_CHANGE   EVENT_MASK(11)

// Adc
#define EVT_SAMPLING        EVENT_MASK(24)
#define EVT_ADC_DONE        EVENT_MASK(25)

// Eternal
#define EVT_EVERY_SECOND    EVENT_MASK(31)
