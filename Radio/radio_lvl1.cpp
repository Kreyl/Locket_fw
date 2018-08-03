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

cc1101_t CC(CC_Setup0);

//#define DBG_PINS

#ifdef DBG_PINS
#define DBG_GPIO1   GPIOA
#define DBG_PIN1    0
#define DBG1_SET()  PinSetHi(DBG_GPIO1, DBG_PIN1)
#define DBG1_CLR()  PinSetLo(DBG_GPIO1, DBG_PIN1)
#define DBG_GPIO2   GPIOB
#define DBG_PIN2    9
#define DBG2_SET()  PinSetHi(DBG_GPIO2, DBG_PIN2)
#define DBG2_CLR()  PinSetLo(DBG_GPIO2, DBG_PIN2)
#else
#define DBG1_SET()
#define DBG1_CLR()
#define DBG2_SET()
#define DBG2_CLR()
#endif

rLevel1_t Radio;
void TmrTimeslotCallback(void *p);

void TmrTxStartCallback(void *p);
void TmrCycleCallback(void *p);

static class RadioTime_t {
private:
    virtual_timer_t TmrTxStart, TmrCycle;
    uint32_t Cycle = 0;
public:
    void OnNewCycleI() {
        Cycle++;
        if(Cycle >= RCYCLE_CNT) Cycle = 0;
        // Start Cycle timer
        chVTSetI(&TmrCycle, (TIMESLOT_CNT * TIMESLOT_DURATION_ST), TmrCycleCallback, nullptr);
        // Generate TX slot
        uint32_t TxSlot = rand() % (TIMESLOT_CNT - 1); // Do not use last timeslot
        if(TxSlot == 0) {    // Tx now
            Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgTimeToTx)); // Enter TX
        }
        else {
            // Enter RX or sleep depending on cycle number
            if(Cycle == 0) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgTimeToRx)); // Enter RX
            else Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgTimeToSleep));
            // Prepare to TX
            chVTSetI(&TmrTxStart, (TxSlot * TIMESLOT_DURATION_ST), TmrTxStartCallback, nullptr);
        }
    }

    void OnTxStartI() {
        Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgTimeToTx)); // Enter TX
    }

    void OnTxRxEndI() {
        if(Cycle == 0) Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgTimeToRx)); // Enter RX
        else Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgTimeToSleep));
    }
} RadioTime;

void TmrTxStartCallback(void *p) {
    chSysLockFromISR();
    RadioTime.OnTxStartI();
    chSysUnlockFromISR();
}
void TmrCycleCallback(void *p) {
    chSysLockFromISR();
    RadioTime.OnNewCycleI();
    chSysUnlockFromISR();
}

void RxCallback() {
    Radio.RMsgQ.SendNowOrExitI(RMsg_t(rmsgPktRx));
}


#if 1 // ================================ Task =================================
static THD_WORKING_AREA(warLvl1Thread, 256);
__noreturn
static void rLvl1Thread(void *arg) {
    chRegSetThreadName("rLvl1");
    Radio.ITask();
}

__noreturn
void rLevel1_t::ITask() {
    while(true) {
        RMsg_t msg = RMsgQ.Fetch(TIME_INFINITE);
        switch(msg.Cmd) {
            case rmsgTimeToTx:
                DBG2_CLR();
                PktTx.ID = ID;
                PktTx.Type = Type;
//                PktTx.Print();
                DBG1_SET();
                CC.Recalibrate();
//                CC.Transmit(&PktTx, RPKT_LEN);
                for(uint32_t N=0; N<4; N++) {
                    if(CC.TransmitWithCCA(&PktTx, RPKT_LEN, -81) == retvOk) {
//                        if(N!=0) Printf("N=%u\r", N);
                        break;
                    }
                    chThdSleep(TIMESLOT_DURATION_ST / 2);
                }
                DBG1_CLR();
                // Tx done
                chSysLock();
                RadioTime.OnTxRxEndI();
                chSysUnlock();
                break;

            case rmsgTimeToRx:
//                Printf("Ttrx\r");
                DBG2_SET();
                CC.ReceiveAsync(RxCallback);
                break;

            case rmsgTimeToSleep:
                DBG2_CLR();
                CC.EnterPwrDown();
                break;

            case rmsgPktRx:
                DBG2_CLR();
                if(CC.ReadFIFO(&PktRx, &Rssi, RPKT_LEN) == retvOk) {  // if pkt successfully received
//                    Printf("%d; ", Rssi);
//                    PktRx.Print();
                    if(Rssi > - 87 and PktRx.ID <= ID_MAX) RxTable.AddOrReplaceExistingPkt(PktRx);
                }
                chSysLock();
                RadioTime.OnTxRxEndI();
                chSysUnlock();
                break;

            case rmsgSetPwr: CC.SetTxPower(msg.Value); break;

            case rmsgSetChnl: CC.SetChannel(msg.Value); break;
        } // switch
    } // while
}

#endif // task

#if 1 // ============================
uint8_t rLevel1_t::Init() {
#ifdef DBG_PINS
    PinSetupOut(DBG_GPIO1, DBG_PIN1, omPushPull);
    PinSetupOut(DBG_GPIO2, DBG_PIN2, omPushPull);
#endif

    RMsgQ.Init();
    if(CC.Init() == retvOk) {
        CC.SetTxPower(CC_PwrPlus5dBm);
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(RCHNL);

        // Thread
        chThdCreateStatic(warLvl1Thread, sizeof(warLvl1Thread), HIGHPRIO, (tfunc_t)rLvl1Thread, NULL);
        chSysLock();
        RadioTime.OnNewCycleI();
        chSchRescheduleS();
        chSysUnlock();
        return retvOk;
    }
    else return retvFail;
}
#endif
