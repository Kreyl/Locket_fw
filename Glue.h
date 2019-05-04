/*
 * Glue.h
 *
 *  Created on: 4 мая 2019 г.
 *      Author: Kreyl
 */

#ifndef GLUE_H_
#define GLUE_H_

void SaveState(uint32_t AState);
void Vibro(uint32_t Duration_ms);
void Flash(uint8_t R, uint8_t G, uint8_t B, uint32_t Duration_ms);
void SendKillingSignal();
void ClearPill();
bool PillWasImmune();
void SaveHP(uint32_t HP);
void SaveMaxHP(uint32_t HP);
void SaveDefaultHP(uint32_t HP);



#endif /* GLUE_H_ */
