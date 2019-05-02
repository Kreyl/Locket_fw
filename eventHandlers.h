#ifndef EVENTSHANDLERS_H_
#define EVENTSHANDLERS_H_

#include "stdio.h"
#include "qhsm.h"

#define Red 0     //delete before release
#define Yellow 1  //delete before release
#define Green 2   //delete before release
#define Blue 3   //delete before release
#define HARD 3
#define LOW 3

//low level functions delete before release
void Vibro(unsigned int force, unsigned int timeout, unsigned int repeat);
void Flash(unsigned int color, unsigned int timeout);
//void Beep();
void SaveHP(unsigned int HP);
void SaveMaxHP(unsigned int MaxHP);
void SaveState(unsigned int State);
void ClearPill(); 
void SendKillingSignal();
void KillingIndication();


//logic functions
void IndicateDamage(unsigned int Damage);
void PillIndicate();
void ShowHitState(QHsm* me);
void UpdateHP(QHsm* me, unsigned int HP);
void UpdateMaxHP(QHsm* me, unsigned int HP);
void IndicateDeath();
void KillingIndication();
void IndicateDeath();

#endif /* EVENTSHANDLERS_H_ */