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

App_t App;

#if 1 // ==== Eeprom ====
// Addresses
#define EE_ADDR_DEVICE_ID       0
#define EE_ADDR_COLOR           4
#endif

// ==== Eternal ====
#define DIP_SW_CNT      6
static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t GetDipSwitch();
static uint8_t ISetID(int32_t NewID);
Eeprom_t EE;
static void ReadIDfromEE();

// Application-specific
static void ReadAndSetupMode();

static TmrKL_t TmrEverySecond {MS2ST(1000), EVT_EVERY_SECOND, tktPeriodic};

Vibro_t Vibro {VIBRO_PIN};
//Beeper_t Beeper {BEEPER_PIN};
LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
//    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();

    // Init OS
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

//    i2c1.Init();
//    PillMgr.Init();

    Led.Init();
//    Led.SetupSeqEndEvt(chThdGetSelfX(), EVT_LED_SEQ_END);
    Led.StartSequence(lsqStart);
    //Led.SetColor(clWhite);

    Vibro.Init();
    Vibro.StartSequence(vsqBrr);

//    Beeper.Init();
//    Beeper.StartSequence(bsqBeepBeep);
//    chThdSleepMilliseconds(702);    // Let it complete the show

//    Btn.Init();
//    Adc.Init();

    // Timers
//    TmrCheckPill.InitAndStart();
    TmrEverySecond.InitAndStart();
    App.SignalEvt(EVT_EVERY_SECOND); // check it now

    if(Radio.Init() == OK) Led.StartSequence(lsqStart);
    else Led.StartSequence(lsqFailure);
    chThdSleepMilliseconds(1800);

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

#if 0 // ==== Pill ====
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

        if(Evt & EVT_RADIO) {   // RxTable is not empty
            uint32_t N = Radio.RxTable.GetCount();
            if(N == 1) Vibro.StartSequence(vsqBrr);
            else if(N == 2) Vibro.StartSequence(vsqBrrBrr);
            else Vibro.StartSequence(vsqBrrBrrBrr);
            Radio.RxTable.Clear();
        }

//        if(Evt & EVT_OFF) {
////            Uart.Printf("Off\r");
//            chSysLock();
//            Sleep::EnableWakeup1Pin();
//            Sleep::EnterStandby();
//            chSysUnlock();
//        }

#if UART_RX_ENABLED
        if(Evt & EVT_UART_NEW_CMD) {
            OnCmd((Shell_t*)&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

    } // while true
}

static uint8_t OldDipSettings = 0xFF;
void ReadAndSetupMode() {
    uint8_t b = GetDipSwitch();
    if(b != OldDipSettings) {
        OldDipSettings = b;
        Uart.Printf("Dip: %02X\r", b);
        App.Mode = (b & 0x20)? modeTx : modeRx;
        if(App.Mode == modeTx) {
            // Setup color
            uint8_t c = (b & 0b011100) >> 2;
            switch(c) {
                case 0b000: Led.StartSequence(lsqRed); break;
                case 0b001: Led.StartSequence(lsqCyan); break;
                case 0b010: Led.StartSequence(lsqYellow); break;
                case 0b011: Led.StartSequence(lsqMagenta); break;
                default: Led.StartSequence(lsqWhite); break;
            }
            // Setup tx power
            c = (b & 0b000011);
            chSysLock();
            switch(c) {
                case 0b00: CC.SetTxPower(CC_PwrMinus27dBm); break;
                case 0b01: CC.SetTxPower(CC_PwrMinus20dBm); break;
                case 0b10: CC.SetTxPower(CC_PwrMinus15dBm); break;
                default:   CC.SetTxPower(CC_PwrMinus10dBm); break;
            }
            chSysUnlock();
        }
        // RX
        else Led.StartSequence(lsqRx);
    }
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
