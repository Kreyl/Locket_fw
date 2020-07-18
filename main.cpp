#include "board.h"
#include "led.h"
#include "beeper.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "kl_i2c.h"
#include "kl_lib.h"
#include "MsgQ.h"
#include "main.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

// EEAddresses
#define EE_ADDR_DEVICE_ID       0

int32_t ID;
static const PinInputSetup_t DipSwPin[DIP_SW_CNT] = { DIP_SW8, DIP_SW7, DIP_SW6, DIP_SW5, DIP_SW4, DIP_SW3, DIP_SW2, DIP_SW1 };
static uint8_t ISetID(int32_t NewID);
void ReadIDfromEE();

// ==== Periphery ====
Beeper_t Beeper {BEEPER_PIN};
LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
LedRGBwPower_t Led2 { LED2_R_PIN, LED2_G_PIN, LED2_B_PIN, LED_EN_PIN };

static LedRGBChunk_t lsqSteady[] = {
        {csSetup, 0, clYellow},
        {csEnd},
};

static LedRGBChunk_t lsqFlash[] = {
        {csSetup, 0, clYellow},
        {csWait, 99},
        {csSetup, 0, clYellow},
        {csEnd},
};

Presser_t Presser;
void BtnIrqHandlerI() { Presser.IrqHandler(); }
const PinIrq_t BtnPin {BTN_PIN, BtnIrqHandlerI};

// ==== Timers ====
static TmrKL_t TmrEverySecond {TIME_MS2I(1000), evtIdEverySecond, tktPeriodic};
static TmrKL_t TmrNoHostTimeout {TIME_MS2I(2000), evtIdNoHost, tktOneShot};
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V5);
    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    EvtQMain.Init();

    // ==== Init hardware ====
    Uart.Init();
    ReadIDfromEE();
    Printf("\r%S %S; ID=%u\r", APP_NAME, XSTRINGIFY(BUILD_TIME), ID);
    Clk.PrintFreqs();

    Led.Init();
    Led2.Init();

    Beeper.Init();
    Beeper.StartOrRestart(bsqBeepBeep);
    chThdSleepMilliseconds(702);    // Let it complete the show
    BtnPin.Init(ttRising);
    BtnPin.EnableIrq(IRQ_PRIO_LOW);
//    Adc.Init();

    // ==== Time and timers ====
    TmrEverySecond.StartOrRestart();
    TmrNoHostTimeout.StartOrRestart();

    // ==== Radio ====
    if(Radio.Init() == retvOk) {
        Led.StartOrRestart(lsqStart);
        Led2.StartOrRestart(lsqStart);
    }
    else {
        Led.StartOrRestart(lsqFailure);
        Led2.StartOrRestart(lsqFailure);
    }
    chThdSleepMilliseconds(1008);

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdEverySecond:
//                Printf("S\r");
//                BtnPin.GenerateIrq();
                break;

            case evtIdHostCmd: {
                chSysLock();
                rPkt_t Pkt = Radio.PktRx;
                chSysUnlock();
                TmrNoHostTimeout.StartOrRestart();
                if(Pkt.Cmd == CMD_SET) {
                    lsqSteady[0].Color.FromRGB(Pkt.R1, Pkt.G1, Pkt.B1);
                    Led.StartOrRestart(lsqSteady);
                    Led2.StartOrRestart(lsqSteady);
                }
                else if(Pkt.Cmd == CMD_FLASH) {
                    lsqFlash[0].Color.FromRGB(Pkt.R1, Pkt.G1, Pkt.B1);
                    lsqFlash[1].Time_ms = Pkt.Wait_ms;
                    lsqFlash[2].Color.FromRGB(Pkt.R2, Pkt.G2, Pkt.B2);
                    Led.StartOrRestart(lsqFlash);
                    Led2.StartOrRestart(lsqFlash);
                }
                else if(Led.GetCurrentSequence() == lsqNoHost) {
                    Led.StartOrRestart(lsqHostNear);
                    Led2.StartOrRestart(lsqHostNear);
                }
            } break;

            case evtIdNoHost:
                Led.StartOrRestart(lsqNoHost);
                Led2.StartOrRestart(lsqNoHost);
                break;

            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;
            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // ITask()

#if 1 // ==== Presser ====
void Presser_t::IrqHandler() {
    if(!WasPressed) {
        TimeOfPress = chVTGetSystemTimeX();
        WasPressed = true;
        WasGet = false;
        PrintfI("W %u\r", TimeOfPress);
    }
    else PrintfI("B\r");
}

int32_t Presser_t::GetTimeAfterPress() {
    WasGet = true;
    return WasPressed? chVTTimeElapsedSinceX(TimeOfPress) : -1;
}

void Presser_t::Reset() {
    if(WasGet) WasPressed = false;
}
#endif

#if 1 // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
//    Uart.Printf("%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) {
        PShell->Ack(retvOk);
    }
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else if(PCmd->NameIs("GetID")) PShell->Reply("ID", ID);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNext<int32_t>(&ID) != retvOk) { PShell->Ack(retvCmdError); return; }
        uint8_t r = ISetID(ID);
        PShell->Ack(r);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif

#if 1 // =========================== ID management =============================
void ReadIDfromEE() {
    ID = EE::Read32(EE_ADDR_DEVICE_ID);  // Read device ID
    if(ID < ID_MIN or ID > ID_MAX) {
        Printf("\rUsing default ID\r");
        ID = ID_DEFAULT;
    }
}

uint8_t ISetID(int32_t NewID) {
    if(NewID < ID_MIN or NewID > ID_MAX) return retvFail;
    uint8_t rslt = EE::Write32(EE_ADDR_DEVICE_ID, NewID);
    if(rslt == retvOk) {
        ID = NewID;
        Printf("New ID: %u\r", ID);
        return retvOk;
    }
    else {
        Printf("EE error: %u\r", rslt);
        return retvFail;
    }
}
#endif
