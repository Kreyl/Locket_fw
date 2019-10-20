#include "health.h"
#include "ability.h"

void DangerTime_Update(Health* me, unsigned int Time);
void HP_Update(Health* me, unsigned int HP);
void MaxHP_Update(Health* me, unsigned int MaxHP);
void Player_Died(Health* me);
void ShowHP(Health* me);
void ChargeTime_Update(Ability *me, unsigned int Time);
void Reset(Health* me);
void IndicateDamage(Health* me);
