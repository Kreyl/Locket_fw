#include "eventHandlers.h"
#include "mHoS.h"
#include "glue.h"
#include <stdio.h>
#include <stdbool.h>


void ShowHP(MHoS* me) {
   Flash(255 - me->CharHP*me->ScaleStep, me->CharHP*me->ScaleStep, 0, SHORT_FLASH_MS);
   return;
   
}

void IndicateDamage(MHoS* me) {
    ShowHP(me);
    Vibro(VIBRO_MS);
}
 
void UpdateHP(MHoS* me, unsigned int HP) {
    if  (HP <= me->MaxHP) {
         me->CharHP = HP;
    } else {
        me->CharHP = me->MaxHP;
    }
    SaveHP(me->CharHP);   
}

void UpdateMaxHP(MHoS* me, unsigned int MaxHP) {
    me->MaxHP = MaxHP;
    SaveMaxHP(me->MaxHP);
    me->ScaleStep = 255/me->MaxHP;
}

void UpdateDefaultHP(MHoS* me, unsigned int DefaultHP) {
	me->DefaultHP = DefaultHP;
	SaveDefaultHP(me->DefaultHP);
	UpdateMaxHP(me, DefaultHP);
	UpdateHP(me, DefaultHP);
}

void Reset(MHoS* me) {
     UpdateMaxHP(me, me->DefaultHP);
     UpdateHP(me, me->DefaultHP);
     me->ScaleStep = 255/me->MaxHP;
	 me->DeathTime = 0;
}

void Double(MHoS* me)  {
     UpdateMaxHP(me, me->DefaultHP*2);
     UpdateHP(me, me->MaxHP);
	 me->DeathTime = 0;
}