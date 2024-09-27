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
CmdUart_t dbg_uart{&CmdUartParams};

LedRGBwPower_t Led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
Vibro_t Vibro {VIBRO_SETUP};
#endif

struct rPkt {
    uint32_t indx = 0; // 0 is all off, [1; 7] are colors
    uint32_t salt = 0;
} __attribute__ ((__packed__));

rPkt pkt_tx;
inline const uint8_t krPktSz = sizeof(rPkt);

inline const uint32_t kSleepDuration = 450UL;

const Color_t colors[8] = { {0,0,0},
        {4,0,0}, {3,3,0}, {0,4,0}, {0,3,3}, {0,0,4}, {3,0,3}, {3,3,3}
};

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
    if(Sleep::WasInStandby() and PinIsLo(BTN1_PIN) and PinIsLo(BTN2_PIN) and PinIsLo(BTN3_PIN)) SleepNow(kSleepDuration);

    // ==== Init Vcore & clock system ====
    SetupVCore(vcore1V2);
    Clk.SetMSI4MHz();
    Clk.UpdateFreqValues();

    // === Init OS ===
    halInit();
    chSysInit();
    EvtQMain.Init();

    // ==== Init hardware ====
    dbg_uart.Init();
    Vibro.Init();
    Led.Init();
    BackupSpc::EnableAccess();
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();

    pkt_tx.indx = BackupSpc::ReadRegister(0);
    if(pkt_tx.indx > 7) pkt_tx.indx = 0;

    if(CC.Init() == retv::Ok) {
        pkt_tx.salt = 0xCa110fEa;
        if(Sleep::WasInStandby()) {
            // Vibrate accordingly
            if(PinIsHi(BTN1_PIN)) {
                Vibro.StartOrRestart(vsqBrr);
                if(pkt_tx.indx < 7) pkt_tx.indx++;
            }
            else if(PinIsHi(BTN2_PIN)) {
                Vibro.StartOrRestart(vsqBrrBrr);
                if(pkt_tx.indx > 0) pkt_tx.indx--;
            }
            else if(PinIsHi(BTN3_PIN)) {
                Vibro.StartOrRestart(vsqBrrBrrBrr);
                pkt_tx.indx = 0;
            }
            BackupSpc::WriteRegister(0, pkt_tx.indx);
            Printf("indx=%u\r", pkt_tx.indx);
            Led.SetColor(colors[pkt_tx.indx]);
            // CC set params
            CC.SetPktSize(krPktSz);
            CC.SetChannel(4); // Same as RX
            CC.SetTxPower(CC_PwrPlus5dBm);
            CC.SetBitrate(CCBitrate100k);
            // Transmit what needed
            while(PinIsHi(BTN1_PIN) or PinIsHi(BTN2_PIN) or PinIsHi(BTN3_PIN)) {
                CC.Recalibrate();
                CC.Transmit(reinterpret_cast<uint8_t*>(&pkt_tx), krPktSz);
                chThdSleepMilliseconds(7);
            }
            CC.EnterPwrDown();
            SleepNow(kSleepDuration); // To repeat transmission soon
        }
        else { // indicate powering on
            Led.StartOrRestart(lsqStart);
            Vibro.StartOrRestart(vsqBrrBrr);
            chThdSleepMilliseconds(999);
            CC.EnterPwrDown();
            SleepNow(kSleepDuration);
        }
    }
    else { // CC failure
        Led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(207);
        SleepNow(2700);
    }
}
