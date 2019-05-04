#ifndef EVENTSHANDLERS_H_
#define EVENTSHANDLERS_H_

#include "stdio.h"
#include "qhsm.h"
#include "mHoS.h"
#include <stdbool.h>


//logic functions
void ShowHP(MHoS *me);
void UpdateHP(MHoS* me, unsigned int HP);
void UpdateMaxHP(MHoS* me, unsigned int HP);
void UpdateDefaultHP(MHoS* me, unsigned int HP);
void Reset(MHoS* me); 
void Double(MHoS* me); 
void IndicateDamage(MHoS* me);
void IndicateDeath();
void KillingIndication();
void IndicateDeath();


#endif /* EVENTSHANDLERS_H_ */