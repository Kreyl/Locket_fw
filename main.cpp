#include "board.h"
#include "led.h"
#include "vibro.h"
#include "beeper.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "kl_i2c.h"
#include "kl_lib.h"
#include "kl_buf.h"
#include "MsgQ.h"
#include "SimpleSensors.h"
#include "buttons.h"
#include "main.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

// ==== Periphery ====
Vibro_t Vibro {VIBRO_SETUP};
LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };

bool IsOn = true;
int32_t ModeToTx = TX_MODE_0;
#endif

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
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
    Led.StartOrRestart(lsqStart);
    Vibro.Init();
    SimpleSensors::Init();
    if(Radio.Init() != retvOk) Led.StartOrRestart(lsqFailure);
    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdButtons:
//                Printf("Btn %u %u\r", Msg.BtnEvtInfo.BtnID, Msg.BtnEvtInfo.Type);
                if(Msg.BtnEvtInfo.BtnID == 0) {
                    IsOn = !IsOn;
                    Radio.PktTx.Indx = IsOn? TX_ON : TX_OFF;
                }
                else if(Msg.BtnEvtInfo.BtnID == 1) {
                    ModeToTx++;
                    if(ModeToTx > TX_MODE_3) ModeToTx = TX_MODE_0;
                    Radio.PktTx.Indx = ModeToTx;
                }
                else if(Msg.BtnEvtInfo.BtnID == 2) {
                    ModeToTx--;
                    if(ModeToTx < TX_MODE_0) ModeToTx = TX_MODE_3;
                    Radio.PktTx.Indx = ModeToTx;
                }
                Vibro.StartOrRestart(vsqBrr);
                Printf("Tx: %d\r", Radio.PktTx.Indx);
                break;

            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;
            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // ITask()

bool MustTx() {
    return (GetBtnState(0) == BTN_HOLDDOWN_STATE)
            or (GetBtnState(1) == BTN_HOLDDOWN_STATE)
            or (GetBtnState(2) == BTN_HOLDDOWN_STATE);
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
