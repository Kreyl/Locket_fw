#include "board.h"
#include "led.h"
#include "vibro.h"
#include "Sequences.h"
#include "kl_lib.h"
#include "cc1101.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
Vibro_t Vibro {VIBRO_SETUP};
#endif

union rPkt_t {
    uint32_t DW32[2];
    struct {
        uint32_t salt;
        uint32_t H;
    };
    rPkt_t& operator = (const rPkt_t &Right) {
        DW32[0] = Right.DW32[0];
        DW32[1] = Right.DW32[1];
        return *this;
    }
} __attribute__ ((__packed__));

rPkt_t pkt_tx;
#define RPKT_LEN    sizeof(rPkt_t)

cc1101_t CC(CC_Setup0);

void SleepNow(uint32_t Delay) {
    chSysLock();
    Iwdg::InitAndStart(Delay);
    Sleep::EnterStandby();
    chSysUnlock();
}

int main(void) {
    // Check if no btn pressed
    PinSetupInput(BTN1_PIN, pudPullDown);
    PinSetupInput(BTN2_PIN, pudPullDown);
    PinSetupInput(BTN3_PIN, pudPullDown);
    // Check if no btn: sleep no long
    if(Sleep::WasInStandby() and PinIsLo(BTN1_PIN) and PinIsLo(BTN2_PIN) and PinIsLo(BTN3_PIN)) SleepNow(450);

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
    Vibro.Init();
    Led.Init();
//    if(!Sleep::WasInStandby()) {
        Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
        Clk.PrintFreqs();
//    }

    if(CC.Init() == retvOk) {
        pkt_tx.salt = 0xCa110fEa;
        if(Sleep::WasInStandby()) {
            // Vibrate accordingly
            if(PinIsHi(BTN1_PIN)) {
                Vibro.StartOrRestart(vsqBrr);
                pkt_tx.H = 0;
                Led.SetColor(clRed);
            }
            else if(PinIsHi(BTN2_PIN)) {
                Vibro.StartOrRestart(vsqBrrBrr);
                pkt_tx.H = 120;
                Led.SetColor(clGreen);
            }
            else if(PinIsHi(BTN3_PIN)) {
                Vibro.StartOrRestart(vsqBrrBrrBrr);
                pkt_tx.H = 240;
                Led.SetColor(clBlue);
            }
            // CC set params
            CC.SetPktSize(RPKT_LEN);
            CC.SetChannel(0); // Same as RX
            CC.SetTxPower(CC_PwrPlus5dBm);
            CC.SetBitrate(CCBitrate100k);
            // Transmit what needed
            while(PinIsHi(BTN1_PIN) or PinIsHi(BTN2_PIN) or PinIsHi(BTN3_PIN)) {
                CC.Recalibrate();
                CC.Transmit(&pkt_tx, RPKT_LEN);
                chThdSleepMilliseconds(7);
            }
            CC.EnterPwrDown();
            SleepNow(270); // To repeat transmission soon
        }
        else { // indicate powering on
            Led.StartOrRestart(lsqStart);
            Vibro.StartOrRestart(vsqBrrBrr);
            chThdSleepMilliseconds(999);
            CC.EnterPwrDown();
            SleepNow(270);
        }
    }
    else { // CC failure
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(207);
        SleepNow(2700);
    }
}
