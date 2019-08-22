/*
 * Glue.h
 *
 *  Created on: 4 мая 2019 г.
 *      Author: Kreyl
 */

#ifndef GLUE_H_
#define GLUE_H_

void Flash(unsigned int R, unsigned int G, unsigned int B, unsigned int Timeout);
void Vibro(unsigned int Timeout);
void SendShining();
void FlashAgony();
void FlashDeath();

void State_Save(unsigned int State);
void Ability_Save(unsigned int Ability);
void PlayerType_Save(unsigned int Type);
void HP_Save(unsigned int HP);
void MaxHP_Save(unsigned int MaxHP);
void DangerTime_Save(unsigned int Time);
void ChargeTime_Save(unsigned int Time);

void SetDefaultColor(uint8_t R, uint8_t G, uint8_t B);
void ClearPill();

// Variables
extern unsigned int ChargeTO;   // ParamID: 1

extern unsigned int DangerTO;   // ParamID: 2
extern unsigned int DefaultHP;  // ParamID: 3
extern unsigned int HPThresh;   // ParamID: 4

extern unsigned int TailorHP;   // ParamID: 5
extern unsigned int LocalHP;    // ParamID: 6
extern unsigned int StalkerHP;  // ParamID: 7
extern unsigned int HealAmount; // ParamID: 8

#endif /* GLUE_H_ */
