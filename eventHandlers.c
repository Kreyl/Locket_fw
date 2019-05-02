#include "eventHandlers.h"
#include "heartOfStorm.h"
#include <stdio.h>

//stub function, delete before release
void Vibro(unsigned int force, unsigned int timeout, unsigned int repeat) {
	printf("Vibro with %i strenth with %i pause for %i times", force, timeout, repeat);
	return;
}

//stub function, delete before release
void Flash(unsigned int color, unsigned int timeout) {
	printf("%i flash for %i ms", color, timeout);
}

//stub function, delete before release
//void Beep() {
//	printf("Beep");
//}

//stub function, delete before release
void SaveHP(unsigned int HP) {
	printf("%i HP Saved\n", HP);
}

//stub function, delete before release
void SaveState(unsigned int State) {
	printf("%i State Saved\n", State);
}

//stub function, delete before release
void SaveMaxHP(unsigned int MaxHP) {
	printf("%i MaxHP Saved\n", MaxHP);
}

//stub function, delete before release
void SendKillingSignal() {
	printf("Killing signal send\n");
}

//stub function, delete before release
void KillingIndication() {
	printf("Indicate send killing signal\n");
}

void ClearPill() {
	printf("Pill cleared");
}

void ShowHitState(QHsm* me) {
    if (((HeartOfStorm*)me)->CharHP >= HP_OK) {
        Flash(Green, FLASH_MS);
    } else if (((HeartOfStorm*)me)->CharHP >= HP_BAD) {
        Flash(Yellow, FLASH_MS);
    } else {
        Flash(Red, FLASH_MS) ;
    }
}    

//stub used change before release
void IndicateDamage(unsigned int Damage) {
    //Flash(Red, FLASH_MS);
    //if (Damage >= DAMAGE_HIGH) {
    //    Vibro(HARD, 300, 3);
    //} else if (Damage > DAMAGE_LOW) {
    //    Vibro(MEDIUM, 300, 2);
    //} else {
    //    Vibro(LOW, 300, 1);
    // }
	Vibro(HARD, 300, 3);
	printf("Damage %i", Damage);
   //Beep();
}
 
void UpdateHP(QHsm* me, unsigned int HP) {
    if  (HP <= ((HeartOfStorm*)me)->MaxHP) {
         ((HeartOfStorm*)me)->CharHP = HP;
    } else {
        ((HeartOfStorm*)me)->CharHP = ((HeartOfStorm*)me)->MaxHP;
    }
    SaveHP(((HeartOfStorm*)me)->CharHP);   
}

void UpdateMaxHP(QHsm* me, unsigned int MaxHP) {
    ((HeartOfStorm*)me)->MaxHP = MaxHP;
    SaveMaxHP(((HeartOfStorm*)me)->MaxHP);
}

void PillIndicate() {
    Flash(Blue, FLASH_MS);
    Vibro(LOW, 300, 1);
}

void IndicateDeath() {
	printf("Dead!!!");
}