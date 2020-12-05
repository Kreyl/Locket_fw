#include "board.h"
#include "led.h"
#include "vibro.h"
#include "beeper.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "kl_i2c.h"
#include "kl_lib.h"
#include "kl_buf.h"
#include "pill.h"
#include "pill_mgr.h"
#include "MsgQ.h"
#include "SimpleSensors.h"
#include "buttons.h"
#include "kl_adc.h"

#include <vector>

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

// ==== Periphery ====
Vibro_t Vibro {VIBRO_SETUP};
#if BEEPER_ENABLED
Beeper_t Beeper {BEEPER_PIN};
#endif

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

// RVar measurement
static TmrKL_t TmrMeasure {TIME_MS2I(99), evtIdMeasure, tktPeriodic};
static void OnMeasurementDone();

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
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();

    Led.Init();
    Vibro.Init();
//    Vibro.StartOrRestart(vsqBrrBrr);

    // ==== Radio ====
    if(Radio.Init() != retvOk) {
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(1008);
    }
//    else Led.StartOrRestart(lsqStart);

    // ADC
    PinSetupAnalog(ADC_RVAR_PIN);
    Adc.Init();
    TmrMeasure.StartOrRestart();

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
#if BUTTONS_ENABLED
            case evtIdButtons:
                Printf("Btn %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                Led.StartOrRestart(lsqBlink);
                switch(Msg.BtnEvtInfo.BtnID) {
                    case 0:
                        if(Msg.BtnEvtInfo.Type == beLongPress) Radio.RMsgQ.SendNowOrExit(RMsg_t(rmsgOnOff));
                        else Radio.RMsgQ.SendNowOrExit(RMsg_t(rmsgFire)); // Short
                        break;
                    case 1: Radio.RMsgQ.SendNowOrExit(RMsg_t(rmsgWhite)); break;
                    case 2: Radio.RMsgQ.SendNowOrExit(RMsg_t(rmsgFlash)); break;
                    default: break;
                }
                break;
#endif

            case evtIdMeasure: Adc.StartMeasurement(); break;
            case evtIdAdcRslt: OnMeasurementDone(); break;

            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;
            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // ITask()

void OnMeasurementDone() {
//    Printf("AdcDone\r");
    if(Adc.FirstConversion) Adc.FirstConversion = false;
    else {
        uint32_t Vadc = Adc.GetResult(RVAR_CHNL);
        Printf("Rvar: %u\r", Vadc);
        Radio.PktTx.Lvl = Vadc;
        uint32_t c = 1 + (Vadc * 99) / 4096;
        Color_t Clr;
        Clr.FromHSV(120, 100, c);
        Led.SetColor(Clr);
    }
}

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


    else PShell->Ack(retvCmdUnknown);
}
#endif
