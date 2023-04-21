#include "board.h"
#include "led.h"
#include "Sequences.h"
#include "cc1101.h"
#include "kl_lib.h"
#include "vibro.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
Vibro_t Vibro {VIBRO_SETUP};
#endif

struct rPkt_t {
    uint32_t DW32[2];
    rPkt_t& operator = (const rPkt_t &Right) {
        DW32[0] = Right.DW32[0];
        DW32[1] = Right.DW32[1];
        return *this;
    }
} __attribute__ ((__packed__));

rPkt_t PktTx;
#define RPKT_LEN    sizeof(PktTx)

cc1101_t CC(CC_Setup0);

int main(void) {
    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
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

    if(CC.Init() == retvOk) {
        if(!Sleep::WasInStandby()) { // indicate powering on
            Led.StartOrRestart(lsqStart);
            chThdSleepMilliseconds(999);
        }
        Vibro.Init();
        // CC params
        CC.SetPktSize(RPKT_LEN);
        CC.SetChannel(0);
        CC.SetTxPower(CC_PwrPlus5dBm);
        CC.SetBitrate(CCBitrate100k);
        PktTx.DW32[0] = 0xCA115EA1;
        PktTx.DW32[1] = 0x0BE17C11;
        // Init button
        PinSetupInput(BTN1_PIN, pudPullDown);
        chThdSleepMilliseconds(18);
        while(PinIsHi(BTN1_PIN)) { // Button is pressed
            Led.StartOrContinue(lsqTX);
            Vibro.StartOrContinue(vsqBrr);
            CC.Recalibrate();
            CC.Transmit((uint8_t*)&PktTx, RPKT_LEN);
        }
    }
    else { // CC failure
        Led.Init();
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(999);
    }

    // Enter sleep
    CC.EnterPwrDown();
    Printf("Enter sleep\r");
    chThdSleepMilliseconds(45);
    chSysLock();
    Sleep::EnableWakeup1Pin();
    Sleep::EnterStandby();
    chSysUnlock();

    while(true); // Will never be here
}
