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

#define EVT_SAMPLING        EVENT_MASK(14)
#define EVT_ADC_DONE        EVENT_MASK(15)
