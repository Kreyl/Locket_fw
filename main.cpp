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
        uint32_t Sign = 0xCa110fEa;
        uint8_t R, G, B;
        uint8_t BtnIndx = 0xFF;
    } __attribute__((__packed__));
    rPkt_t& operator = (const rPkt_t &Right) {
        DW32[0] = Right.DW32[0];
        DW32[1] = Right.DW32[1];
        return *this;
    }
} __attribute__ ((__packed__));

rPkt_t PktTx;
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
    if(Sleep::WasInStandby() and PinIsLo(BTN1_PIN) and PinIsLo(BTN2_PIN)) SleepNow(450); // no btn, sleep no long

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
//    if(!Sleep::WasInStandby()) {
        Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
        Clk.PrintFreqs();
//    }

    if(CC.Init() == retvOk) {
        if(Sleep::WasInStandby()) {
            // Vibrate accordingly
            if(PinIsHi(BTN1_PIN)) Vibro.StartOrRestart(vsqBrr);
            else if(PinIsHi(BTN2_PIN)) Vibro.StartOrRestart(vsqBrrBrr);
            // CC set params
            CC.SetPktSize(RPKT_LEN);
            CC.SetChannel(7); // Same as RX
            CC.SetTxPower(CC_PwrPlus5dBm);
            CC.SetBitrate(CCBitrate100k);
            // Transmit what needed
            while(PinIsHi(BTN1_PIN) or PinIsHi(BTN2_PIN)) {
                PktTx.BtnIndx = PinIsHi(BTN1_PIN)? 0 : 1;
                CC.Recalibrate();
                CC.Transmit(&PktTx, RPKT_LEN);
            }
            CC.EnterPwrDown();
            SleepNow(270); // To repeat transmission soon
        }
        else { // indicate powering on
            Led.Init();
            Led.StartOrRestart(lsqStart);
            Vibro.StartOrRestart(vsqBrrBrr);
            chThdSleepMilliseconds(999);
            CC.EnterPwrDown();
            SleepNow(270);
        }
    }
    else { // CC failure
        Led.Init();
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(207);
        SleepNow(2700);
    }
}
