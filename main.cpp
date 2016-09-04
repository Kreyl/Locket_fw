/*
 * main.cpp
 *
 *  Created on: 20 февр. 2014 г.
 *      Author: g.kruglov
 */

#include "main.h"
#include "SimpleSensors.h"
#include "buttons.h"
#include "board.h"
#include "led.h"
#include "vibro.h"
#include "beeper.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "pill_mgr.h"
#include "kl_adc.h"

// ==== Eternal ====
App_t App;

// EEAddresses
#define EE_ADDR_DEVICE_ID       0
#define EE_ADDR_COLOR           4

static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static void ReadAndSetupMode();
static uint8_t ISetID(int32_t NewID);
Eeprom_t EE;
static void ReadIDfromEE();

// ==== Periphery ====
Vibro_t Vibro {VIBRO_PIN};
//Beeper_t Beeper {BEEPER_PIN};
LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

// ==== Application-specific ====
//static const BaseChunk_t *pVibroSeqToPerform = nullptr;

// ==== Timers ====
static TmrKL_t TmrEverySecond   {MS2ST(1000), EVT_EVERY_SECOND, tktPeriodic};
static TmrKL_t TmrIndicationOff {MS2ST(3600), EVT_INDICATION_OFF, tktOneShot};

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
//    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    App.InitThread();

    // ==== Init hardware ====
    Uart.Init(115200, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN);
    ReadIDfromEE();
    Uart.Printf("\r%S %S; ID=%u\r", APP_NAME, BUILD_TIME, App.ID);
//    Uart.Printf("ID: %X %X %X\r", GetUniqID1(), GetUniqID2(), GetUniqID3());
//    if(Sleep::WasInStandby()) {
//        Uart.Printf("WasStandby\r");
//        Sleep::ClearStandbyFlag();
//    }
    Clk.PrintFreqs();
//    RandomSeed(GetUniqID3());   // Init random algorythm with uniq ID

    // === LED ===
    Led.Init();
//    Led.SetupSeqEndEvt(chThdGetSelfX(), EVT_LED_SEQ_END);
//    Led.StartSequence(lsqStart);
    //Led.SetColor(clWhite);

    // === Vibro ===
    Vibro.Init();
    Vibro.SetupSeqEndEvt(chThdGetSelfX(), EVT_VIBRO_END);
    Vibro.StartSequence(vsqBrr);

#if BEEPER_ENABLED // === Beeper ===
    Beeper.Init();
    Beeper.StartSequence(bsqBeepBeep);
    chThdSleepMilliseconds(702);    // Let it complete the show
#endif

//    Btn.Init();
//    Adc.Init();

#if PILL_ENABLED // === Pill ===
    i2c1.Init();
    PillMgr.Init();
    TmrCheckPill.InitAndStart();
#endif

    TmrEverySecond.InitAndStart();
    TmrIndicationOff.Init();
    App.SignalEvt(EVT_EVERY_SECOND); // check it now

    if(Radio.Init() == OK) Led.StartSequence(lsqStart);
    else Led.StartSequence(lsqFailure);
    chThdSleepMilliseconds(1008);

    // Main cycle
    App.ITask();
}

__noreturn
void App_t::ITask() {
    while(true) {
        __unused eventmask_t Evt = chEvtWaitAny(ALL_EVENTS);
        if(Evt & EVT_EVERY_SECOND) {
            ReadAndSetupMode();
        }

#if PILL_ENABLED // ==== Pill ====
        if(Evt & EVT_PILL_CHECK) {
            PillMgr.Check();
            switch(PillMgr.State) {
                case pillJustConnected:
                    Uart.Printf("Pill: %d %d\r", PillMgr.Pill.TypeInt32, PillMgr.Pill.ChargeCnt);
                    break;
//                case pillJustDisconnected: Uart.Printf("Pill Discon\r"); break;
                default: break;
            }
        }
#endif

#if 0 // ==== Led sequence end ====
        if(Evt & EVT_LED_SEQ_END) {
        }
#endif

        if(Evt & EVT_RADIO) {
            if(App.Mode == modeDetectorRx) {
                // Something received
                if(Led.IsIdle()) Led.StartSequence(lsqCyan);
                TmrIndicationOff.Restart(); // Reset off timer
            }
//                const BaseChunk_t *pVibroSeqNew;
//                uint8_t N = Radio.RxTable.GetCount();
////                Uart.Printf("Cnt=%u\r", N);
//                switch(N) {
//                    case 0:  pVibroSeqNew = nullptr;   break;
//                    case 1:  pVibroSeqNew = vsqBrr;    break;
//                    case 2:  pVibroSeqNew = vsqBrrBrr; break;
//                    default: pVibroSeqNew = vsqBrrBrrBrr; break;
//                } // switch
//                // If was not vibrating, start to vibrate
//                if(pVibroSeqToPerform == nullptr and pVibroSeqNew != nullptr) Vibro.StartSequence(pVibroSeqNew);
//                pVibroSeqToPerform = pVibroSeqNew;
//            }
//            else pVibroSeqToPerform = nullptr; // Stop vibration
//            Radio.RxTable.Clear();
        }

        if(Evt & EVT_INDICATION_OFF) {
            Led.StartSequence(lsqOff);
        }

//        if(Evt & EVT_OFF) {
////            Uart.Printf("Off\r");
//            chSysLock();
//            Sleep::EnableWakeup1Pin();
//            Sleep::EnterStandby();
//            chSysUnlock();
//        }

#if 0 // ==== Vibro seq end ====
        if(Evt & EVT_VIBRO_END) {
            // Restart vibration (or start new one) if needed
            if(pVibroSeqToPerform != nullptr) Vibro.StartSequence(pVibroSeqToPerform);
        }
#endif

#if UART_RX_ENABLED
        if(Evt & EVT_UART_NEW_CMD) {
            OnCmd((Shell_t*)&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

    } // while true
}

void ReadAndSetupMode() {
    static uint8_t OldDipSettings = 0xFF;
    uint8_t b = GetDipSwitch();
    if(b == OldDipSettings) return;
    // Something has changed
    OldDipSettings = b;
    Uart.Printf("Dip: %02X\r", b);
    // ==== Binding ====
    if((b & 0b100000) == 0) {
        App.Mode = modeBinding;
        Led.StartSequence(lsqBindingStart);
    }
    // ==== Detector ====
    else {
        if(b & 0b010000) {
            App.Mode = modeDetectorTx;
            Led.StartSequence(lsqDetectorTxStart);
        }
        else {
            App.Mode = modeDetectorRx;
            Led.StartSequence(lsqDetectorRxStart);
        }
    }
    // ==== Setup TX power ====
    uint8_t pwrIndx = (b & 0b000111);
    chSysLock();
    CC.SetTxPower(CCPwrTable[pwrIndx]);
    chSysUnlock();
}


#if UART_RX_ENABLED // ================= Command processing ====================
void App_t::OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(OK);
    }

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", App.ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNextNumber(&dw32) != OK) { PShell->Ack(CMD_ERROR); return; }
        uint8_t r = ISetID(dw32);
        PShell->Ack(r);
    }

    else PShell->Ack(CMD_UNKNOWN);
}
#endif

#if 1 // =========================== ID management =============================
void ReadIDfromEE() {
    App.ID = EE.Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(App.ID < ID_MIN or App.ID > ID_MAX) {
        Uart.Printf("\rUsing default ID\r");
        App.ID = ID_DEFAULT;
    }
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return FAILURE;
    uint8_t rslt = EE.Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == OK) {
        App.ID = NewID;
        Uart.Printf("New ID: %u\r", App.ID);
        return OK;
    }
    else {
        Uart.Printf("EE error: %u\r", rslt);
        return FAILURE;
    }
}
#endif

// ====== DIP switch ======
uint8_t GetDipSwitch() {
    uint8_t Rslt = 0;
    for(int i=0; i<DIP_SW_CNT; i++) PinSetupInput(DipSwPin[i].PGpio, DipSwPin[i].Pin, DipSwPin[i].PullUpDown);
    for(int i=0; i<DIP_SW_CNT; i++) {
        if(!PinIsHi(DipSwPin[i].PGpio, DipSwPin[i].Pin)) Rslt |= (1 << i);
        PinSetupAnalog(DipSwPin[i].PGpio, DipSwPin[i].Pin);
    }
    return Rslt;
}
