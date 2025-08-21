#include "board.h"
#include "led.h"
#include "vibro.h"
#include "Sequences.h"
#include "kl_lib.h"
#include "cc1101.h"

#pragma region // ======================== Variables and defines ========================
// Forever
extern const char *kBuildTime, *kBuildCfgName;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> evt_q_main;
static const UartParams_t kCmdUartParams(115200, CMD_UART_PARAMS);
CmdUart uart { &kCmdUartParams };

LedRGBwPower_t led { LED_R_PIN, LED_G_PIN, LED_B_PIN, LED_EN_PIN };
Vibro_t vibro { VIBRO_SETUP };
#pragma endregion

#pragma region // =========================== Radio Packet ===============================
#pragma pack(push, 1)
struct rPkt {
    int32_t value;
    union {
        uint32_t dw32;
        struct {
            uint8_t from;
            uint8_t to;
            uint8_t cmd;
            int8_t rssi;
        };
    };
    rPkt& operator = (const rPkt &right) {
        value = right.value;
        dw32 = right.dw32;
        return *this;
    }
    void Print() {
        Printf("from %u; to %u; cmd %u; value=%d; rssi=%d\r", from, to, cmd, value, rssi);
    }
};
#pragma pack(pop)

inline constexpr const uint8_t kRecipientBroadcast = 0xFF;
inline constexpr const uint8_t kRPktSz = sizeof(rPkt);

inline constexpr const uint8_t kCmdSetVolume = 0x51;
inline constexpr const uint8_t kCmdRestart = 0x57;


rPkt pkt_tx;
#pragma endregion

inline const uint32_t kSleepDuration = 450UL;

const int32_t kHVolumeMax = 0;   // Red
const int32_t kHVolumeMin = 240; // Blue
const int32_t kVolumeMax = 100;
const int32_t kVolumeStep = 10;


cc1101_t CC(CC_Setup0);

void SleepNow(uint32_t delay) {
    chSysLock();
    Iwdg::InitAndStart(delay);
    Sleep::EnterStandby();
    chSysUnlock();
}

void main(void) {
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
    evt_q_main.Init();

    // ==== Init hardware ====
    uart.Init();
    vibro.Init();
    led.Init();

    // Get saved volume
    BackupSpc::EnableAccess();
    pkt_tx.value = BackupSpc::ReadRegister(0);
    if(pkt_tx.value > kVolumeMax) pkt_tx.value = kVolumeMax;
    pkt_tx.from = 0;
    pkt_tx.to = kRecipientBroadcast;

    Printf("\r%S %S; volume=%d\r", APP_NAME, kBuildTime, pkt_tx.value);
    Printf("pkt sz %u\r", kRPktSz);
    Clk.PrintFreqs();

    if(CC.Init() == retv::Ok) {
        if(Sleep::WasInStandby()) {
            // Vibrate accordingly
            if(PinIsHi(BTN1_PIN)) { // Volume up
                vibro.StartOrRestart(vsqBrr);
                pkt_tx.value += kVolumeStep;
                if(pkt_tx.value > kVolumeMax) pkt_tx.value = kVolumeMax;
                pkt_tx.cmd = kCmdSetVolume;
            }
            else if(PinIsHi(BTN2_PIN)) { // Restart
                vibro.StartOrRestart(vsqBrrBrr);
                pkt_tx.cmd = kCmdRestart;
            }
            else if(PinIsHi(BTN3_PIN)) { // Volume down
                vibro.StartOrRestart(vsqBrrBrrBrr);
                pkt_tx.value -= kVolumeStep;
                if(pkt_tx.value < 0) pkt_tx.value = 0;
                pkt_tx.cmd = kCmdSetVolume;
            }
            BackupSpc::WriteRegister(0, pkt_tx.value);
            Printf("Volume=%d\r", pkt_tx.value);
            pkt_tx.Print();

            // Set color
            ColorHSV_t hsv { 0, 100, 100 };
            hsv.H = Proportion<int32_t>(0, kVolumeMax, kHVolumeMin, kHVolumeMax, pkt_tx.value);
            if(hsv.H > 360) hsv.H = 360;
            led.SetColor(hsv.ToRGB());

            // CC set params
            CC.SetPktSize(kRPktSz);
            CC.SetChannel(0); // Same as RX
            CC.SetTxPower(CC_PwrPlus5dBm);
            CC.SetBitrate(CCBitrate100k);

            // Transmit what needed
            while(PinIsHi(BTN1_PIN) or PinIsHi(BTN2_PIN) or PinIsHi(BTN3_PIN)) {
                CC.Recalibrate();
                CC.Transmit(reinterpret_cast<uint8_t*>(&pkt_tx), kRPktSz);
                chThdSleepMilliseconds(7);
            }
            CC.EnterPwrDown();
            SleepNow(kSleepDuration); // To repeat transmission soon
        }
        else { // indicate powering on
            led.StartOrRestart(lsqStart);
            vibro.StartOrRestart(vsqBrrBrr);
            chThdSleepMilliseconds(999);
            CC.EnterPwrDown();
            SleepNow(kSleepDuration);
        }
    }
    else { // CC failure
        led.StartOrRestart(lsqFailure);
        chThdSleepMilliseconds(207);
        SleepNow(2700);
    }
}
