#include "health.h"
#include "eventHandlers.h"
#include "player_type.h"
#include <stdio.h>
#include "qhsm.h"
#include "glue.h"

void Player_Died(Health* me) {
    State_Save(DEAD);
    HP_Update(me, 0);
    Vibro(DEATH_TO_S*1000/3);
    Flash(255, 0, 0, DEATH_TO_S*1000);
    SIMPLE_DISPATCH(the_ability, DISABLE);
    SIMPLE_DISPATCH(the_player_type, DIE);
}

void ShowHP(Health* me) {
    Flash(255 - (me->CharHP)*255/me->MaxHP, (me->CharHP*255)/me->MaxHP, 0, FLASH_MS/3);
}

void DangerTime_Update(Health* me, unsigned int Time) {
    me->DangerTime = Time;
    DangerTime_Save(Time);
}


void HP_Update(Health* me, unsigned int HP) {
	printf("Try to set %iHP\n", HP);
    if  (HP <= me->MaxHP) {
        if(HP > 0) {
             me->CharHP = HP;
        } else {
             me->CharHP = 0;
        }
    } else {
         me->CharHP = me->MaxHP;
    }
    HP_Save(me->CharHP);
}

void Reset(Health* me) {
    HP_Update(me, me->MaxHP);
    DangerTime_Update(me, 0);
}

void MaxHP_Update(Health* me, unsigned int MaxHP) {
    me->MaxHP = MaxHP;
    MaxHP_Save(MaxHP);
    HP_Update(me, MaxHP);
}

void IndicateDamage(Health* me) {
    ShowHP(me);
    Vibro(VIBRO_MS);
}

void ChargeTime_Update(Ability *me, unsigned int Time) {
	me->ChargeTime = Time;
	ChargeTime_Save(Time);
}
