/*
 * radio_lvl1.cpp
 *
 *  Created on: Nov 17, 2013
 *
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

int32_t RssiOffset = 0;
void GetRssiOffset();

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
            CC.SetTxPower(CC_PwrMinus20dBm);
            for(int i=0; i<4; i++) {
                CC.Recalibrate();
                CC.Transmit(&PktTx, RPKT_LEN);
                chThdSleepMilliseconds(99);
            }
        }
        // ==== Rx ====
        CC.Recalibrate();
        if(CC.Receive(360, &PktRx, RPKT_LEN, &Rssi) == retvOk) {
            Printf("From: %u; To: %u; TrrID: %u; PktID: %u; Cmd: %u; Rssi: %d\r\n", PktRx.From, PktRx.To, PktRx.TransmitterID, PktRx.PktID, PktRx.Cmd, Rssi);
//            Led.StartOrRestart(lsqBlinkB);
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
                PktTx.LocketParam.ParamID = PktRx.LocketParam.ParamID;
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
                break;

            case rcmdLocketExplode:
                EvtQMain.SendNowOrExit(EvtMsg_t(evtIdShineOrderHost));
                break;

            case rcmdLocketDieChoosen:
                EvtQMain.SendNowOrExit(EvtMsg_t(evtIdShinePktMutant));
                break;

            default: PktTx.Pong.Reply = retvCmdError; break;
        } // switch
        // Transmit pkt
        if(PktRx.PktID != PKTID_DO_NOT_RETRANSMIT) chThdSleepMilliseconds(999); // Let network to calm down
        CC.SetTxPower(CC_PwrPlus5dBm);
        systime_t Start = chVTGetSystemTimeX();
        while(true) {
            int32_t TxTime_ms = (int32_t)324 - (int32_t)(TIME_I2MS(chVTTimeElapsedSinceX(Start)));
            if(TxTime_ms <= 0) break;
            CC.Recalibrate();
            CC.Transmit(&PktTx, RPKT_LEN);
            //chThdSleepMilliseconds(6);
        }
    }
    else { // for everyone!
        switch(PktRx.Cmd) {
            case rcmdBeacon: // Damage pkt from lustra
                if(PktRx.From >= LUSTRA_MIN_ID and PktRx.From <= LUSTRA_MAX_ID) {
                    // Add to accumulator. Averaging is done in main thd
                    int32_t Indx = PktRx.From - LUSTRA_MIN_ID;
                    if(Indx >= 0 and Indx < LUSTRA_CNT) {
                        Radio.RxData[Indx].Cnt++;
                        Radio.RxData[Indx].Summ += Rssi + RssiOffset;
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
        GetRssiOffset();
        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        return retvOk;
    }
    else return retvFail;
}

#define OFFSET_TABLE_CNT    ID_MAX
static const int32_t IdOffsetTable[OFFSET_TABLE_CNT][2] = {
        {1,11},
        {2,5},
        {3,2},
		{4,-4},
		{5,11},
        {6,5},
        {7, 9},
        {8, 12},
	    {9, 9},
		{10,5},
		{11,7},
        {12,4},
        {13,0},
		{14,6},
		{15,0},
        {16,3},
        {17,7},
        {18,4},
	    {19,10},
		{20,9},
    	{21,6},
        {22,12},
		{23, 7},
		{24,4},
		{25,2},
        {26,12},
        {27,-2},
        {28,6},
	    {29,4},
		{30,5},
		{31,2},
		{32,9},
		{33,7},
		{34,6},
		{35,6},
        {36,5},
        {37,9},
        {38,11},
	    {39,-2},
		{40,12},
		{41, 2},
	    {42,1},
	    {43,9},
	    {44,9},
		{45, 4},
        {46,4},
        {47,3},
        {48,2},
	    {49,9},
		{50,2},
		{51,7},
        {52, 7},
        {53, -1},
		{54,10},
		{55,0},
        {56,4},
        {57,1},
        {58,-1},
	    {59,11},
		{60,4},
		{61,1},
		{62,1},
        {63,8},
		{64,0},
		{65,5},
        {66,14},
        {67,8},
        {68,6},
	    {69, 5},
		{70,8},
		{71,-2},
        {72,4},
        {73,0},
		{44,9},
		{75,4},
        {76,0},
        {77,-7},
        {78, 3},
	    {79,3},
		{80,0},
		{81,0},
        {82,-1},
        {83,-4},
		{84,1},
		{85,-1},
        {86,2},
        {87,-1},
        {88,0},
	    {89,0},
		{90,-2},
		{91,0},
        {92,-2},
        {93,2},
		{94,5},
		{95,-1},
        {96,3},
        {97,3},
        {98,-1},
	    {99,9},
		{100,3},
    	{101,1},
        {102,-3},
        {103,-1},
		{104,6},
		{105,-1},
        {106,3},
        {107,-1},
        {108,-1},
	    {109,-6},
		{110,-2},
    	{111,-3},
        {112,3},
        {113,-3},
		{114,3},
		{115,-1},
        {116,4},
        {117,4},
        {118,9},
	    {119,0},
		{120,4},
    	{121,11},
        {122,10},
        {123,10},
		{124,1},
		{125,5},
        {126,-2},
        {127,-2},
        {128,-2},
	    {129,-1},
		{130,10},
    	{131,-2},
        {132,11},
        {133,0},
		{134,0},
		{135,3},
        {136,8},
        {137,0},
        {138,0},
	    {139,0},
		{140,0},
    };

void GetRssiOffset() {
    for(int i=0; i<OFFSET_TABLE_CNT; i++) {
        if(IdOffsetTable[i][0] == ID) {
            RssiOffset = IdOffsetTable[i][1];
            break;
        }
    }
}
#endif
