/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: kreyl
 */

#include "radio_lvl1.h"
#include "cc1101.h"
#include "uart.h"
#include "main.h"

#include "led.h"
#include "beeper.h"
#include "Sequences.h"
#include "Glue.h"

extern LedRGBwPower_t Led;
extern Beeper_t Beeper;

cc1101_t CC(CC_Setup0);

//#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOB
#define DBG_PIN1    10
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
#define DBG_GPIO2   GPIOB
#define DBG_PIN2    9
#define DBG2_SET()  PinSetHi(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinSetLo(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#endif

rLevel1_t Radio;
extern int32_t ID;
void ProcessRCmd();

static rPkt_t PktTx, PktRx;
static int8_t Rssi;

#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    while(true) {
        // ==== TX if needed ====
        RMsg_t Msg = Radio.RMsgQ.Fetch(TIME_IMMEDIATE);
        if(Msg.Cmd == R_MSG_SEND_KILL) {
            PktTx.From = ID;
            PktTx.To = 0;
            PktTx.TransmitterID = ID;
            PktTx.Cmd = rcmdLocketDieAll;
            PktTx.PktID = PKTID_DO_NOT_RETRANSMIT;
            PktTx.Die.RssiThr = RSSI_FOR_MUTANT;
            for(int i=0; i<4; i++) {
                CC.Recalibrate();
                CC.Transmit(&PktTx, RPKT_LEN);
                chThdSleepMilliseconds(99);
            }
        }
        // ==== Rx ====
        CC.Recalibrate();
        uint8_t RxRslt = CC.Receive(180, &PktRx, RPKT_LEN, &Rssi);
        if(RxRslt == retvOk) {
//            Printf("Rssi=%d\r", Rssi);
//            Printf("%u: Thr: %d; Pwr: %u; Rssi: %d\r", RxPkt.From, RxPkt.RssiThr, RxPkt.Value, Rssi);
            ProcessRCmd();
        }
    } // while true
}

void ProcessRCmd() {
    if(PktRx.To == ID) { // For us
        // Common preparations
        PktTx.From = ID;
        PktTx.To = PktRx.From;
        PktTx.TransmitterID = ID;
        PktTx.Cmd = rcmdPong;
        if(PktRx.PktID != PKTID_DO_NOT_RETRANSMIT) { // Increase PktID if not PKTID_DO_NOT_RETRANSMIT
            if(PktRx.PktID >= PKTID_TOP_VALUE) PktTx.PktID = 1;
            else PktTx.PktID = PktRx.PktID + 1;
        }
        else PktTx.PktID = PKTID_DO_NOT_RETRANSMIT;
        PktTx.Pong.MaxLvlID = PktRx.TransmitterID;
        PktTx.Pong.Reply = retvOk;

        // Command processing
        switch(PktRx.Cmd) {
            case rcmdPing: break; // Do not fall into default

            case rcmdLocketSetParam:
                switch(PktRx.LocketParam.ParamID) {
                    case 1: ChargeTO = PktRx.LocketParam.Value; break;
                    case 2: DangerTO = PktRx.LocketParam.Value; break;
                    case 3: DefaultHP = PktRx.LocketParam.Value; break;
                    case 4: HPThresh = PktRx.LocketParam.Value; break;
                    case 5: TailorHP = PktRx.LocketParam.Value; break;
                    case 6: LocalHP = PktRx.LocketParam.Value; break;
                    case 7: StalkerHP = PktRx.LocketParam.Value; break;
                    case 8: HealAmount = PktRx.LocketParam.Value; break;
                    default: PktTx.Pong.Reply = retvBadValue; break;
                } // switch
                break;

            case rcmdLocketGetParam:
                PktTx.Cmd = rcmdLocketGetParam;
                switch(PktRx.LocketParam.ParamID) {
                    case 1: PktTx.LocketParam.Value = ChargeTO; break;
                    case 2: PktTx.LocketParam.Value = DangerTO; break;
                    case 3: PktTx.LocketParam.Value = DefaultHP; break;
                    case 4: PktTx.LocketParam.Value = HPThresh; break;
                    case 5: PktTx.LocketParam.Value = TailorHP; break;
                    case 6: PktTx.LocketParam.Value = LocalHP; break;
                    case 7: PktTx.LocketParam.Value = StalkerHP; break;
                    case 8: PktTx.LocketParam.Value = HealAmount; break;
                    default:
                        PktTx.Cmd = rcmdPong;
                        PktTx.Pong.Reply = retvBadValue;
                        break;
                }
                break;

            case rcmdScream:
                Beeper.StartOrRestart(bsqSearch);
                Led.StartOrRestart(lsqSearch);
                chThdSleepMilliseconds(108);
                break;

            case rcmdLocketExplode:
                EvtQMain.SendNowOrExit(EvtMsg_t(evtIdShineOrderHost));
                break;

            case rcmdLocketDieChoosen:
                EvtQMain.SendNowOrExit(EvtMsg_t(evtIdShinePktMutant));
                break;

            default: PktTx.Pong.Reply = retvCmdError; break;
        } // switch
    }
    else { // for everyone!
        switch(PktRx.Cmd) {
            case rcmdBeacon: // Damage pkt from lustra
                if(PktRx.From >= LUSTRA_MIN_ID and PktRx.From <= LUSTRA_MAX_ID) {
                    // Add to accumulator. Averaging is done in main thd
                    int32_t Indx = PktRx.From - LUSTRA_MIN_ID;
                    if(Indx >= 0 and Indx < LUSTRA_CNT) {
                        Radio.RxData[Indx].Cnt++;
                        Radio.RxData[Indx].Summ += Rssi;
                        Radio.RxData[Indx].RssiThr = PktRx.Beacon.RssiThr;
                        Radio.RxData[Indx].Damage = PktRx.Beacon.Damage;
                    }
                }
                break;

            case rcmdLocketDieAll:
                EvtQMain.SendNowOrExit(EvtMsg_t(evtIdShinePktMutant));
                break;

            default: break;
        } // switch
    } // else
}
#endif // task

void rLevel1_t::TryToSleep(uint32_t SleepDuration) {
    if(SleepDuration >= MIN_SLEEP_DURATION_MS) CC.EnterPwrDown();
    chThdSleepMilliseconds(SleepDuration);
}

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    for(int i=0; i<LUSTRA_CNT; i++) {
        RxData[i].Cnt = 0;
        RxData[i].Summ = 0;
    }

    RMsgQ.Init();
    if(CC.Init() == retvOk) {
        CC.SetTxPower(CC_PwrMinus20dBm);
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(1);
//        CC.EnterPwrDown();
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}
#endif
